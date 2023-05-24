#include "BpReceiveStream.h"
#include "Logger.h"
#include <boost/process.hpp>

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

#define FFMPEG_SDP_HEADER "data:application/sdp;,"

BpReceiveStream::BpReceiveStream(size_t numCircularBufferVectors, const std::string& rtpDestHostname, const uint16_t rtpDestPort,
        uint16_t maxOutgoingRtpPacketSizeBytes, std::string ffmpegCommand, uint8_t outductMode, std::string fileNameToSave) 
    : BpSinkPattern(), 
    m_numCircularBufferVectors(numCircularBufferVectors),
    m_outgoingRtpPort(rtpDestPort),
    m_maxOutgoingRtpPacketSizeBytes(maxOutgoingRtpPacketSizeBytes),
    socket(io_service),
    m_ffmpegCommand(ffmpegCommand),
    m_outductMode(outductMode)
{
    
    m_maxOutgoingRtpPayloadSizeBytes = m_maxOutgoingRtpPacketSizeBytes - sizeof(rtp_header);
    m_outgoingDtnRtpPtr = std::make_shared<DtnRtp>(UINT64_MAX);
    m_processingThread = boost::make_unique<boost::thread>(boost::bind(&BpReceiveStream::ProcessIncomingBundlesThread, this)); 
    
    m_incomingBundleQueue.set_capacity(m_numCircularBufferVectors);

    
    if (m_outductMode == UDP_OUTDUCT)
    {
        m_udpBatchSenderPtr = std::make_shared<UdpBatchSender>();
        m_udpBatchSenderPtr->SetOnSentPacketsCallback(boost::bind(&BpReceiveStream::OnSentRtpPacketCallback, this, 
                boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
        m_udpBatchSenderPtr->Init(rtpDestHostname, rtpDestPort);
        m_udpEndpoint = m_udpBatchSenderPtr->GetCurrentUdpEndpoint();
    
        socket.open(boost::asio::ip::udp::v4());
    } else if (m_outductMode == GSTREAMER_APPSRC_OUTDUCT)
    {
        m_gstreamerAppSrcOutductPtr = boost::make_unique<GStreamerAppSrcOutduct>(fileNameToSave);
        SetGStreamerAppSrcOutductInstance(m_gstreamerAppSrcOutductPtr.get());
    }

}

BpReceiveStream::~BpReceiveStream()
{
    LOG_INFO(subprocess) << "Calling BpReceiveStream deconstructor";
    m_running = false;
    
    m_gstreamerAppSrcOutductPtr.reset();
    m_udpBatchSenderPtr->Stop();

    Stop();

    LOG_INFO(subprocess) << "m_totalRtpPacketsReceived: " << m_totalRtpPacketsReceived;
    LOG_INFO(subprocess) << "m_totalRtpPacketsSent: " << m_totalRtpPacketsSent;
    LOG_INFO(subprocess) << "m_totalRtpBytesSent: " << m_totalRtpBytesSent; 
    LOG_INFO(subprocess) << "m_totalRtpPacketFailedToSend: " << m_totalRtpPacketsFailedToSend;
    LOG_INFO(subprocess) << "m_incomingBundleQueue.size(): " << m_incomingBundleQueue.size();
}

void BpReceiveStream::ProcessIncomingBundlesThread()
{
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    static bool firstPacket = true;

    padded_vector_uint8_t rtpFrame;
    rtpFrame.reserve(m_maxOutgoingRtpPacketSizeBytes);

    while (m_running) 
    {
        bool notInWaitForNewPacketState = TryWaitForIncomingDataAvailable(timeout);
        if (notInWaitForNewPacketState) {

            boost::mutex::scoped_lock lock(m_incomingQueueMutex);
            
            padded_vector_uint8_t &incomingBundle = m_incomingBundleQueue.front(); // process front of queue
            // LOG_DEBUG(subprocess) << "bundle is size " << incomingBundle.size();
           
            if (firstPacket) {
                firstPacket = false;
                m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingBundleQueue.front().data(), USE_INCOMING_SEQ); // starts us at same sequence number
            }

            size_t offset = 0; 
            while (1)
            {
                size_t rtpPacketLength;
                memcpy(&rtpPacketLength, incomingBundle.data() + offset, sizeof(size_t)); // get length of next rtp packet
                offset += sizeof(size_t);
                size_t rtpPayloadLength = rtpPacketLength - sizeof(rtp_header); // size of the rtp payload without the rtp header
                
                uint8_t * rtpPacketLocation = incomingBundle.data() + offset;
                
                // LOG_DEBUG(subprocess) << "Begin sending next packet of length " << rtpPacketLength;
                
                // size our frame so we can insert the packet
                rtpFrame.resize(rtpPacketLength);

                // update header (everything execpt sequence number & marker bit from the incoming packet)
                m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) rtpPacketLocation, USE_INCOMING_SEQ);

                // copy outgoing RtpDtn session header into outbound frame
                memcpy(rtpFrame.data(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));
                offset += sizeof(rtp_header); 

                // copy payload into outbound frame
                memcpy(rtpFrame.data() + sizeof(rtp_header),  // start after header
                        rtpPacketLocation + sizeof(rtp_header), // start after header
                        rtpPayloadLength); // dont include header size
                offset += rtpPayloadLength;

                // rtp_frame * frame = (rtp_frame *)  rtpPacketLocation;
                // frame->print_header();

                if (m_outductMode == UDP_OUTDUCT) {
                    SendUdpPacket(rtpFrame);
                } else if (m_outductMode == GSTREAMER_APPSRC_OUTDUCT) {
                    m_gstreamerAppSrcOutductPtr->PushRtpPacketToGStreamer(rtpFrame); // gets taken
                }

                m_outgoingDtnRtpPtr->IncSequence(); // for next frame

                m_totalRtpPacketsReceived++;

                // LOG_DEBUG(subprocess) << "offset " << offset << "  incoming size" << incomingBundle.size();
                if (offset == incomingBundle.size())
                    break;
            }
            
            // LOG_DEBUG(subprocess) << "Popping";
            m_incomingBundleQueue.pop_front();

        }
    }
}

int BpReceiveStream::SdpPacketHandle(const padded_vector_uint8_t& vec)
{
    static bool printedSdp = false;
    LOG_INFO(subprocess) << "Got SDP File information";
    std::string sdpFile((char * )&vec[sizeof(uint64_t)], vec.size() - sizeof(uint64_t));
    
    if (printedSdp == false) {
        printedSdp = true;
        LOG_INFO(subprocess) << "Sdp File: \n" << sdpFile;
    }

    if (TranslateBpSdpToInSdp(sdpFile) == 0) 
    {
        if (m_ffmpegCommand.length() == 0) 
        {
            // Save sdp to text file for input to ffmpeg later on in runscript
            boost::filesystem::ofstream file("HDTN_TO_IN_SDP.sdp");
            file << m_sdpFileString;
            file.close();
        } 
        else 
        { // we want to execute ffmpeg in our program, not externally
            if (m_executedFfmpeg == false) 
            {
                ExecuteFFmpegInstance();
                m_executedFfmpeg = true;
            } 
        } 
    } else {
        return -1;
    }

    return 0;

}

// Data from BpSourcePattern comes in through here
bool BpReceiveStream::ProcessPayload(const uint8_t *data, const uint64_t size)
{
    padded_vector_uint8_t vec(size);
    memcpy(vec.data(), data, size);
    uint64_t flag ;
    memcpy(&flag, vec.data(), sizeof(uint64_t));

    if (flag == SDP_FILE_STR_HEADER) {
        std::cout << flag << std::endl; 
        if (SdpPacketHandle(vec) == 0) 
            return true;
    } 


    {
        boost::mutex::scoped_lock lock(m_incomingQueueMutex); // lock mutex 
        m_incomingBundleQueue.push_back(std::move(vec));
    }
    m_incomingQueueCv.notify_one();

    return true;
}

// we need to change the port number to the outgoing rtp port
int BpReceiveStream::TranslateBpSdpToInSdp(std::string sdp)
{
    std::string newSdp;
    
    // "c" field
    newSdp.append("c=IN IP4 ");
    newSdp.append(m_udpEndpoint.address().to_string());
    newSdp.append("\n");

    // "m" field
    newSdp.append("m=");
    if (sdp.find("m=video") != std::string::npos)
        newSdp.append("video ");
    if (sdp.find("m=audio") != std::string::npos)
        newSdp.append("audio ");
    newSdp.append(std::to_string(m_udpEndpoint.port()));
    newSdp.append(" ");
    
    // append the rest of the original SDP message
    size_t rtpLocation = sdp.find("RTP/AVP"); // sdp protocol for RTP
    if (rtpLocation == std::string::npos) 
    {
        LOG_ERROR(subprocess) << "Invalid SDP file";
        return -1;
    }

    std::string sdpSubString = sdp.substr(rtpLocation, UINT64_MAX);
    newSdp.append(sdpSubString);
    
    m_sdpFileString = newSdp;
    LOG_INFO(subprocess) << "Translated IN SDP:\n" << m_sdpFileString;
    
    return 0;
}

int BpReceiveStream::ExecuteFFmpegInstance()
{
    std::string input("-i ");
    std::string newline("\n");
    std::string finalCommand;
    finalCommand = m_ffmpegCommand;

    std::string sdp = " -i \"";
    sdp.append(FFMPEG_SDP_HEADER);
    sdp.append(m_sdpFileString);
    sdp.pop_back(); // remove new line
    sdp.pop_back(); // remove new line
    sdp.append("\" ");
    
    size_t inputParamLocation = finalCommand.find("-vcodec") ;
    if (inputParamLocation != std::string::npos)
    {
        finalCommand.insert(inputParamLocation, sdp);
        LOG_INFO(subprocess) << "Final ffmpeg command: \n" << finalCommand;
        
        boost::process::child ffmpegProcess(finalCommand);
        ffmpegProcess.detach();
    }


    return 0;
}

bool BpReceiveStream::TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration &timeout)
{
    if (m_incomingBundleQueue.size() == 0) { // if empty, we wait
        return GetNextIncomingPacketTimeout(timeout);
    }
    return true; 
}

bool BpReceiveStream::GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_incomingQueueMutex);
    if ((m_incomingBundleQueue.size() == 0)) {
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


int BpReceiveStream::SendUdpPacket(padded_vector_uint8_t& message) {


    m_totalRtpBytesSent += socket.send_to(boost::asio::buffer(message), m_udpEndpoint);
    // LOG_DEBUG(subprocess) << "Send RTP Packet";
	// try {
	// } catch (const boost::system::system_error& ex) {
	// 	// Exception thrown!
	// 	// Examine ex.code() and ex.what() to see what went wrong!
    //     m_totalRtpPacketsFailedToSend++;
    //     LOG_ERROR(subprocess) << "Failed to send code: " << ex.code() << " what:" << ex.what();
	// 	return -1;
	// }

    m_totalRtpPacketsSent++;
	return 0;
}