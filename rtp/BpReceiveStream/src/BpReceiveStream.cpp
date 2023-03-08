#include "BpReceiveStream.h"
#include "Logger.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

BpReceiveStream::BpReceiveStream(size_t numCircularBufferVectors, const std::string& remoteHostname, const uint16_t remotePort, uint16_t maxOutgoingRtpPacketSizeBytes) : BpSinkPattern(), 
        m_numCircularBufferVectors(numCircularBufferVectors),
        m_outgoingRtpPort(remotePort),
        m_maxOutgoingRtpPacketSizeBytes(maxOutgoingRtpPacketSizeBytes)
{
    m_maxOutgoingRtpPayloadSizeBytes = m_maxOutgoingRtpPacketSizeBytes - sizeof(rtp_header);
    m_outgoingDtnRtpPtr = std::make_shared<DtnRtp>(UINT64_MAX);
    m_processingThread = boost::make_unique<boost::thread>(boost::bind(&BpReceiveStream::ProcessIncomingBundlesThread, this)); 
    
    m_incomingCircularPacketQueue.set_capacity(m_numCircularBufferVectors);

    m_udpBatchSenderPtr = std::make_shared<UdpBatchSender>();
    m_udpBatchSenderPtr->SetOnSentPacketsCallback(boost::bind(&BpReceiveStream::OnSentRtpPacketCallback, this, 
            boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
    m_udpBatchSenderPtr->Init(remoteHostname, remotePort);
    m_udpEndpoint = m_udpBatchSenderPtr->GetCurrentUdpEndpoint();

}

BpReceiveStream::~BpReceiveStream()
{
    m_running = false;

    m_udpBatchSenderPtr->Stop();
    Stop();

    LOG_INFO(subprocess) << "m_totalRtpPacketsReceived: " << m_totalRtpPacketsReceived;
    LOG_INFO(subprocess) << "m_totalRtpPacketsSent: " << m_totalRtpPacketsSent;
    LOG_INFO(subprocess) << "m_totalRtpPacketsQueued: " << m_totalRtpPacketsQueued;

}

void BpReceiveStream::ProcessIncomingBundlesThread()
{
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    static bool firstPacket = true;

    while (m_running) 
    {
        bool notInWaitForNewPacketState = TryWaitForIncomingDataAvailable(timeout);

        if (notInWaitForNewPacketState) {
           
            boost::mutex::scoped_lock lock(m_incomingQueueMutex);
            
            padded_vector_uint8_t &incomingPacket = m_incomingCircularPacketQueue.front(); // process front of queue

            // split front of incoming queue into the minimum number of RTP packets possible
            size_t totalBytesPayload = incomingPacket.size() - sizeof(rtp_header) ; // get size of the incoming rtp frame payload
            size_t bytesPayloadRemaining = totalBytesPayload; 
            size_t numPacketsToTransferPayload = ceil((float) (totalBytesPayload/m_maxOutgoingRtpPacketSizeBytes)); 


            // use these to export to UdpBatchSend
            std::shared_ptr<std::vector<UdpSendPacketInfo> > udpSendPacketInfoVecPtr = 
                    std::make_shared<std::vector<UdpSendPacketInfo> >(numPacketsToTransferPayload);  
            std::vector<UdpSendPacketInfo>& udpSendPacketInfoVec = *udpSendPacketInfoVecPtr;

            // this is where we copy incoming frames into, reserve max space right now for efficiency
            std::vector<std::vector<uint8_t>> rtpFrameVec(numPacketsToTransferPayload);
            for (auto vec=rtpFrameVec.begin(); vec!=rtpFrameVec.end(); vec++)  {
                vec->reserve(m_maxOutgoingRtpPacketSizeBytes);
            }

            if (firstPacket) {
                firstPacket = false;
                m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingCircularPacketQueue.front().data(), USE_INCOMING_SEQ); // starts us at same sequence number
            }

            size_t offset = sizeof(rtp_header); // start first byte after header 
            for (size_t i = 0; i < numPacketsToTransferPayload; i++)
            {
                size_t nextPacketSize;
                if (bytesPayloadRemaining > m_maxOutgoingRtpPacketSizeBytes) {
                    nextPacketSize = m_maxOutgoingRtpPacketSizeBytes;
                } else {
                    nextPacketSize = bytesPayloadRemaining;
                }
                size_t bytesPayloadToCopy = nextPacketSize- sizeof(rtp_header);


                // update header (everything execpt sequence number from the incoming packet)
                m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) incomingPacket.data(), USE_OUTGOING_SEQ);
                // copy outgoing RtpDtn session header into every frame
                memcpy(rtpFrameVec[i].data(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));
                m_outgoingDtnRtpPtr->IncSequence(); // for next frame


                // copy incrementing payload into frame
                memcpy(rtpFrameVec[i].data()+sizeof(rtp_header), incomingPacket.data()+offset, bytesPayloadToCopy);
                offset += bytesPayloadToCopy;

                rtp_frame * frame = (rtp_frame * ) rtpFrameVec[i].data();
                frame->print_header();

                bytesPayloadRemaining -= bytesPayloadToCopy;  // only take off the payload bytes

                // append to outgoing data (cheap "copy")
                udpSendPacketInfoVec[i].constBufferVec.resize(1);
                udpSendPacketInfoVec[i].constBufferVec[0] = boost::asio::buffer(rtpFrameVec[i]);
            }

            // batch send
            m_udpBatchSenderPtr->QueueSendPacketsOperation_ThreadSafe(std::move(udpSendPacketInfoVecPtr), numPacketsToTransferPayload); // data is stolen from rtpFrameVec




            m_incomingCircularPacketQueue.pop_front();
        }
    }
}

bool BpReceiveStream::ProcessPayload(const uint8_t *data, const uint64_t size)
{
    padded_vector_uint8_t vec(size);
    memcpy(vec.data(), data, size);

    {
        boost::mutex::scoped_lock lock(m_incomingQueueMutex);// lock mutex 
        m_incomingCircularPacketQueue.push_back(std::move(vec));
    }
    m_incomingQueueCv.notify_one();
    m_totalRtpPacketsReceived++;

    // LOG_DEBUG(subprocess) << "Got bundle of size " << size;
    // rtp_frame * frame= (rtp_frame *) data;
    // frame->print_header();
    return true;
}

bool BpReceiveStream::TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration &timeout)
{
    if (m_incomingCircularPacketQueue.size() == 0) { // if empty, we wait
        return GetNextIncomingPacketTimeout(timeout);
    }
    return true; 
}

bool BpReceiveStream::GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_incomingQueueMutex);
    if ((m_incomingCircularPacketQueue.size() == 0)) {
        m_incomingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }
    
    return true;
}

void BpReceiveStream::OnSentRtpPacketCallback(bool success, std::shared_ptr<std::vector<UdpSendPacketInfo>> &udpSendPacketInfoVecSharedPtr, const std::size_t numPacketsSent)
{
    if (success) 
    {
        m_totalRtpPacketsSent += numPacketsSent;
        m_totalRtpBytesSent += udpSendPacketInfoVecSharedPtr->size();
    } else {
        LOG_ERROR(subprocess) << "Failed to send RTP packet";
    }
}
