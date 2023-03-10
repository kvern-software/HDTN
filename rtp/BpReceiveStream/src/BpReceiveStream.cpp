#include "BpReceiveStream.h"
#include "Logger.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

BpReceiveStream::BpReceiveStream(size_t numCircularBufferVectors, const std::string& remoteHostname, const uint16_t remotePort, uint16_t maxOutgoingRtpPacketSizeBytes) : BpSinkPattern(), 
        m_numCircularBufferVectors(numCircularBufferVectors),
        m_outgoingRtpPort(remotePort),
        m_maxOutgoingRtpPacketSizeBytes(maxOutgoingRtpPacketSizeBytes),
        socket(io_service)
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
   
    socket.open(boost::asio::ip::udp::v4());

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
            double packetsToTransfer = ((double) totalBytesPayload) / ((double) m_maxOutgoingRtpPacketSizeBytes);
            size_t numPacketsToTransferPayload = ceil(packetsToTransfer); 


            // use these to export to UdpBatchSend
            // std::shared_ptr<std::vector<UdpSendPacketInfo> > udpSendPacketInfoVecPtr = 
                    // std::make_shared<std::vector<UdpSendPacketInfo> >(numPacketsToTransferPayload);  
            // std::vector<UdpSendPacketInfo>& udpSendPacketInfoVec = *udpSendPacketInfoVecPtr;

            // this is where we copy incoming frames into, reserve max space right now for efficiency
            // std::vector<std::vector<uint8_t>> rtpFrameVec(numPacketsToTransferPayload);
            // for (auto vec=rtpFrameVec.begin(); vec!=rtpFrameVec.end(); vec++)  {
            //     vec->reserve(m_maxOutgoingRtpPacketSizeBytes);
            // }
            
            // LOG_DEBUG(subprocess) << "Sending " << numPacketsToTransferPayload  << " packets";

            if (firstPacket) {
                firstPacket = false;
                m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingCircularPacketQueue.front().data(), USE_INCOMING_SEQ); // starts us at same sequence number
            }

            size_t offset = sizeof(rtp_header); // start first byte after header 
            for (size_t i = 0; i < numPacketsToTransferPayload; i++)
            {
                size_t nextPacketSize;
                if ( (bytesPayloadRemaining + sizeof(rtp_header)) >= m_maxOutgoingRtpPacketSizeBytes) {
                    nextPacketSize = m_maxOutgoingRtpPacketSizeBytes;
                } else {
                    nextPacketSize = bytesPayloadRemaining + sizeof(rtp_header);
                }
                size_t bytesPayloadToCopy = nextPacketSize - sizeof(rtp_header);

                std::vector<uint8_t> rtpFrame(nextPacketSize);

                // update header (everything execpt sequence number & marker bit from the incoming packet)
                m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) incomingPacket.data(), USE_INCOMING_SEQ);
                // copy outgoing RtpDtn session header into every frame
                memcpy(rtpFrame.data(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));
                m_outgoingDtnRtpPtr->IncSequence(); // for next frame


                // copy incrementing payload into frame
                memcpy(rtpFrame.data()+sizeof(rtp_header), incomingPacket.data()+offset, bytesPayloadToCopy);
                
                SendUdpPacket(rtpFrame);
                offset += bytesPayloadToCopy;

                // std::cout << "frame" << std::endl;
                // rtp_frame * frame = (rtp_frame * ) rtpFrame.data();
                // frame->print_header();
                
                // std::cout << rtpFrameVec.size() << " packets" << std::endl;
                bytesPayloadRemaining -= bytesPayloadToCopy;  // only take off the payload bytes

                // append to outgoing data (cheap "copy")
                // udpSendPacketInfoVec[i].constBufferVec.resize(1);
                // udpSendPacketInfoVec[i].constBufferVec[0] = boost::asio::buffer(rtpFrameVec[i]);

            }
    // std::cout << rtpFrameVec.size() << " packets" << std::endl;
            // we need to ensure that the batch sender actually sends our data before exiting so it is not lost
            // {
            //     m_sentPacketsSuccess = false;
            //     boost::mutex::scoped_lock lock(m_sentPacketsMutex); //must lock before checking the flag

            //     m_udpBatchSenderPtr->QueueSendPacketsOperation_ThreadSafe(std::move(udpSendPacketInfoVecPtr), numPacketsToTransferPayload); // data is stolen from rtpFrameVec

            //     while (m_sentPacketsSuccess == false) {
            //         LOG_DEBUG(subprocess) << "Waiting for sucessful send";
            //         m_cvSentPacket.timed_wait(lock, timeout);
            //         LOG_DEBUG(subprocess) << "Got sucessful send";
            //     }
            // }
            // SendUdpPacket(rtpFrameVec);
            
            // LOG_DEBUG(subprocess) << "Popping";
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



bool BpReceiveStream::TryWaitForSuccessfulSend(const boost::posix_time::time_duration &timeout)
{
    if (!m_sentPacketsSuccess) { 
        return GetSuccessfulSendTimeout(timeout);
    }
    return true; 
}

bool BpReceiveStream::GetSuccessfulSendTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_sentPacketsMutex);
    if (!m_sentPacketsSuccess) {
        m_cvSentPacket.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }
    
    return true;
}



void BpReceiveStream::OnSentRtpPacketCallback(bool success, std::shared_ptr<std::vector<UdpSendPacketInfo>> &udpSendPacketInfoVecSharedPtr, const std::size_t numPacketsSent)
{
    m_sentPacketsSuccess = true;
    m_cvSentPacket.notify_one();

    if (success) 
    {
        m_totalRtpPacketsSent += numPacketsSent;
        m_totalRtpBytesSent += udpSendPacketInfoVecSharedPtr->size();
        LOG_DEBUG(subprocess) << "Sent " <<  numPacketsSent << " packets. Sent " << udpSendPacketInfoVecSharedPtr->size() << " bytes";
    } else {
        LOG_ERROR(subprocess) << "Failed to send RTP packet";
    }
}


int BpReceiveStream::SendUdpPacket(const std::vector<uint8_t>& message) {


	try {
		// Open the socket, socket's destructor will
		// automatically close it.
		// And send the string... (synchronous / blocking)
		socket.send_to(boost::asio::buffer(message), m_udpEndpoint);
	
	} catch (const boost::system::system_error& ex) {
		// Exception thrown!
		// Examine ex.code() and ex.what() to see what went wrong!
        LOG_ERROR(subprocess) << "Failed to send code: " << ex.code() << " what:" << ex.what();
		return -1;
	}
	return 0;
}