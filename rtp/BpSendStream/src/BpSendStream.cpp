#include "BpSendStream.h"
#include "Logger.h"
#include "ThreadNamer.h"
#include <boost/process.hpp>

int
make_named_socket (const char *filename)
{
  struct sockaddr_un name;
  int sock;
  size_t size;

  /* Create the socket. */
  sock = socket (AF_LOCAL, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEPORT) failed");
  
  
  
  /* Bind a name to the socket. */
  name.sun_family = AF_LOCAL;
  strncpy (name.sun_path, filename, sizeof (name.sun_path));
  name.sun_path[sizeof (name.sun_path) - 1] = '\0';

  /* The size of the address is
     the offset of the start of the filename,
     plus its length (not including the terminating null byte).
     Alternatively you can just do:
     size = SUN_LEN (&name);
 */
  size = (offsetof (struct sockaddr_un, sun_path)
          + strlen (name.sun_path));

    // this is potentially unsafe
    unlink(filename);


  if (bind (sock, (struct sockaddr *) &name, size) < 0)
    {
      perror ("bind");
      exit (EXIT_FAILURE);
    }

  return sock;
}





static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

BpSendStream::BpSendStream(uint8_t intakeType, size_t maxIncomingUdpPacketSizeBytes, uint16_t incomingRtpStreamPort, size_t numCircularBufferVectors, 
        size_t maxOutgoingBundleSizeBytes, bool enableRtpConcatentation, std::string sdpFile, uint64_t sdpInterval_ms, uint16_t numRtpPacketsPerBundle,
        std::string fileToStream) : BpSourcePattern(),
    m_intakeType(intakeType),
    m_running(true),
    m_numCircularBufferVectors(numCircularBufferVectors),
    m_maxIncomingUdpPacketSizeBytes(maxIncomingUdpPacketSizeBytes),
    m_incomingRtpStreamPort(incomingRtpStreamPort),
    m_maxOutgoingBundleSizeBytes(maxOutgoingBundleSizeBytes),
    m_enableRtpConcatentation(enableRtpConcatentation),
    m_sdpFileStr(sdpFile),
    m_sdpInterval_ms(sdpInterval_ms),
    m_numRtpPacketsPerBundle(numRtpPacketsPerBundle),
    m_fileToStream(fileToStream)
{
    m_currentFrame.reserve(m_maxOutgoingBundleSizeBytes);

    /**
     * DtnRtp objects keep track of the RTP related paremeters such as sequence number and stream identifiers. 
     * The information in the header can be used to enhance audio/video (AV) playback.
     * Here, we have a queue for the incoming and outgoing RTP streams. 
     * BpSendStream has the ability to reduce RTP related overhead by concatenating RTP 
     * packets. The concatenation of these packets follows the guidelines presented in the CCSDS 
     * red book "SPECIFICATION FOR RTP AS TRANSPORT FOR AUDIO AND VIDEO OVER DTN CCSDS 766.3-R-1"
    */
    m_incomingDtnRtpPtr = std::make_shared<DtnRtp>(m_maxIncomingUdpPacketSizeBytes);
    m_outgoingDtnRtpPtr = std::make_shared<DtnRtp>(m_maxOutgoingBundleSizeBytes);

    m_processingThread = boost::make_unique<boost::thread>(boost::bind(&BpSendStream::ProcessIncomingBundlesThread, this)); 
    m_sdpThread = boost::make_unique<boost::thread>(boost::bind(&BpSendStream::SdpTimerThread, this)); 

    m_ioServiceThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_ioService));
    ThreadNamer::SetIoServiceThreadName(m_ioService, "ioServiceBpUdpSink");

    m_incomingCircularPacketQueue.set_capacity(numCircularBufferVectors);
    m_outgoingCircularBundleQueue.set_capacity(numCircularBufferVectors);   

    if (m_intakeType == HDTN_APPSINK_INTAKE) {
        SetCallbackFunction(boost::bind(&BpSendStream::WholeBundleReadyCallback, this, boost::placeholders::_1));
        m_gstreamerAppSinkIntakePtr = boost::make_unique<GStreamerAppSinkIntake>(m_fileToStream);
    } else if (m_intakeType == HDTN_UDP_INTAKE) {
        m_bundleSinkPtr = std::make_shared<UdpBundleSink>(m_ioService, m_incomingRtpStreamPort, 
        boost::bind(&BpSendStream::WholeBundleReadyCallback, this, boost::placeholders::_1),
        numCircularBufferVectors, 
        maxIncomingUdpPacketSizeBytes, 
        boost::bind(&BpSendStream::DeleteCallback, this));
    } else if (m_intakeType == HDTN_FD_INTAKE) {
    /**
     * File descriptor input
    */
        // InitFdSink();
        // m_fdThread = boost::make_unique<boost::thread>(boost::bind(&BpSendStream::FdSinkThread, this)); 
        // ExecuteGst("test");

    } else if (m_intakeType == HDTN_TCP_INTAKE) {

    } else {
        // LOG_ERROR(subprocess) << "Unrecognized intake option. Aborting";
        // exit(-1);
    }



    /**
     * TCP
    */
    // boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string("127.0.0.1"), m_incomingRtpStreamPort);
    // m_tcpAcceptorPtr = boost::make_unique<boost::asio::ip::tcp::acceptor>(m_ioService, ep);
    // StartTcpAccept();
}

BpSendStream::~BpSendStream()
{
    m_running = false;

    // shut down whatever sink is running
    m_gstreamerAppSinkIntakePtr.reset();

    m_bundleSinkPtr.reset();

    if (m_tcpAcceptorPtr->is_open()) {
        try {
            m_tcpAcceptorPtr->close();
        }
        catch (const boost::system::system_error & e) {
            LOG_ERROR(subprocess) << "Error closing TCP Acceptor in StcpInduct::~StcpInduct:  " << e.what();
        }
    }


    if (m_ioServiceThreadPtr) {
        m_ioServiceThreadPtr->join();   
        m_ioServiceThreadPtr.reset(); //delete it
    }



    Stop();

    LOG_INFO(subprocess) << "m_incomingCircularPacketQueue.size(): " << m_incomingCircularPacketQueue.size();
    LOG_INFO(subprocess) << "m_outgoingCircularBundleQueue.size(): " << m_outgoingCircularBundleQueue.size();
    LOG_INFO(subprocess) << "m_totalRtpPacketsReceived: " << m_totalRtpPacketsReceived;
    LOG_INFO(subprocess) << "m_totalRtpPacketsSent: " << m_totalRtpPacketsSent ;
    LOG_INFO(subprocess) << "m_totalRtpPacketsQueued: " << m_totalRtpPacketsQueued;
    LOG_INFO(subprocess) << "m_totalConcatenationsPerformed" << m_totalConcatenationsPerformed;
    LOG_INFO(subprocess) << "m_totalIncomingCbOverruns: " << m_totalIncomingCbOverruns;
    LOG_INFO(subprocess) << "m_totalOutgoingCbOverruns: " << m_totalOutgoingCbOverruns;

    // LOG_INFO(subprocess) << "m_totalMarkerBits" << m_totalMarkerBits;
    // LOG_INFO(subprocess) << "m_totalTimestampChanged" << m_totalTimestampChanged;

    shutdown(m_fd, 2);

}

void BpSendStream::WholeBundleReadyCallback(padded_vector_uint8_t &wholeBundleVec)
{
    {
        boost::mutex::scoped_lock lock(m_incomingQueueMutex);// lock mutex 
        // LOG_DEBUG(subprocess) << "Pushing frame into incoming queue";
        if (m_incomingCircularPacketQueue.full())
            m_totalIncomingCbOverruns++;

        m_incomingCircularPacketQueue.push_back(std::move(wholeBundleVec));  // copy out bundle to local queue for processing
        // rtp_header * header = (rtp_header *) wholeBundleVec.data();
    }
    m_incomingQueueCv.notify_one();
}


/**
 * - RTP UDP packets are delivered, not bundles. The UDP bundle sink name scheme is inherited, but we are really receiving UDP packets, not bundles.
*/
void BpSendStream::ProcessIncomingBundlesThread()
{
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));

    while (m_running) {
        bool notInWaitForNewBundlesState = TryWaitForIncomingDataAvailable(timeout);

        if (notInWaitForNewBundlesState) {
            // LOG_DEBUG(subprocess) << "Processing front of incoming queue"; 
            // boost::this_thread::sleep_for(boost::chrono::milliseconds(250));
            
            m_incomingQueueMutex.lock();
            padded_vector_uint8_t incomingRtpFrame(std::move(m_incomingCircularPacketQueue.front()));
            m_incomingCircularPacketQueue.pop_front();
            m_incomingQueueMutex.unlock();

            rtp_packet_status_t packetStatus = m_incomingDtnRtpPtr->PacketHandler(incomingRtpFrame, (rtp_header *) m_currentFrame.data());
                
                switch(packetStatus) {
            // //         /**
            // //          * For the first valid frame we receive assign the CSRC, sequence number, and generic status bits by copying in the first header
            // //          * Note - after this point, it is likely and intended that the sequence of the incoming and outgoing DtnRtp objects diverge.
            // //         */
                    case RTP_FIRST_FRAME:
                        m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) incomingRtpFrame.data(), USE_INCOMING_SEQ);
                        CreateFrame(incomingRtpFrame);
                        break;

            //         case RTP_CONCATENATE:
            //             if (m_enableRtpConcatentation) {
            //                 // concatenation may fail if the bundle size is less than requested rtp frame, if so RTP_PUSH_PREVIOUS_FRAME is performed 
            //                 Concatenate(incomingRtpFrame); 
            //                 break;
            //             } else {
            //                 PushFrame();
            //                 CreateFrame();
            //                 break;
            //             }
                        
                    case RTP_PUSH_PREVIOUS_FRAME: // push current frame and make incoming frame the current frame
                        PushFrame();
                        CreateFrame(incomingRtpFrame);
                        break;

            //         case RTP_OUT_OF_SEQ: 
            //             PushFrame(); 
            //             CreateFrame();
            //             break;

                    case RTP_INVALID_HEADER: // discard incoming data
                    case RTP_MISMATCH_SSRC: // discard incoming data
                    case RTP_INVALID_VERSION: // discard incoming data
                    default:
                        LOG_ERROR(subprocess) << "Unknown return type " << packetStatus;
                }

                m_totalRtpPacketsReceived++;
        }
    }
}
// Copy in our outgoing Rtp header and the next rtp frame payload
void BpSendStream::CreateFrame(padded_vector_uint8_t &incomingRtpFrame)
{
    m_currentFrame.resize(incomingRtpFrame.size()); 

    memcpy(&m_currentFrame.front(), incomingRtpFrame.data(), incomingRtpFrame.size());
    
    // fill header with outgoing Dtn Rtp header information
    // this is tranlating from incoming DtnRtp session to outgoing DtnRtp session
    // memcpy(&m_currentFrame.front(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));
    // memcpy(&m_currentFrame.front() + sizeof(rtp_header), 
            // &incomingRtpFrame + sizeof(rtp_header),  // skip the incoming header for our outgoing header instead
            // incomingRtpFrame.size() - sizeof(rtp_header));

    m_offset = incomingRtpFrame.size(); // assumes that this had a header that we skipped

    m_outgoingDtnRtpPtr->IncNumConcatenated();
}

// if the current frame + incoming packet payload < bundle size, then concatenate
void BpSendStream::Concatenate(padded_vector_uint8_t &incomingRtpFrame)
{
    if ((m_offset + incomingRtpFrame.size() - sizeof(rtp_header)) < m_maxOutgoingBundleSizeBytes) { // concatenate if we have enough space
        m_currentFrame.resize(m_currentFrame.size() + incomingRtpFrame.size() - sizeof(rtp_header)); // enlarge buffer
        
        memcpy(&m_currentFrame.at(m_offset), // skip to our previous end of buffer
                &incomingRtpFrame + sizeof(rtp_header), // skip incoming header, use outgoing header
                incomingRtpFrame.size() - sizeof(rtp_header)); 
        m_offset += incomingRtpFrame.size() - sizeof(rtp_header); // did not copy header

        m_outgoingDtnRtpPtr->IncNumConcatenated();
        m_totalConcatenationsPerformed++;

        // LOG_DEBUG(subprocess) << "Number of RTP packets in current frame: " << m_outgoingDtnRtpPtr->GetNumConcatenated();
    } else { // not enough space to concatenate, send separately
        // LOG_DEBUG(subprocess) << "Not enough space to concatenate, sending as separate bundle.";
        PushFrame();
        CreateFrame(incomingRtpFrame);
    }
}

// Add current frame to the outgoing queue to be bundled and sent
void BpSendStream::PushFrame()
{
    // prepend the payload size to the payload here
    size_t rtpFrameSize = m_currentFrame.size();
    padded_vector_uint8_t rtpPacketSizeAndPacket(m_currentFrame.size() + sizeof(size_t));
    memcpy(rtpPacketSizeAndPacket.data(), &rtpFrameSize, sizeof(size_t));
    memcpy(rtpPacketSizeAndPacket.data() + sizeof(size_t), m_currentFrame.data(), m_currentFrame.size());
    m_currentFrame.resize(0);


    // move into rtp packet group
    m_outgoingRtpPacketQueue.push(std::move(rtpPacketSizeAndPacket)); // new element at end 
    m_rtpBytesInQueue += rtpFrameSize + sizeof(size_t);

    // LOG_DEBUG(subprocess) << "Inserted " << rtpFrameSize << " byte rtp packet into queue";
    // LOG_DEBUG(subprocess) << "m_rtpBytesInQueue = " <<  m_rtpBytesInQueue;

    // Outgoing DtnRtp session needs to update seq status since we just sent an RTP frame. This is the seq number used for the next packet
    // m_outgoingDtnRtpPtr->IncSequence();
    // m_outgoingDtnRtpPtr->ResetNumConcatenated();
    // m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) incomingRtpFrame.data(), USE_OUTGOING_SEQ); // updates our next header to have the correct ts, fmt, ext, m ect.

    m_offset = 0;
    
    m_totalRtpPacketsQueued++;

    if (m_outgoingRtpPacketQueue.size() == m_numRtpPacketsPerBundle) // we should move this packet group into the bundle queue
    {
        PushBundle();
    }
}

void BpSendStream::PushBundle()
{
    // LOG_DEBUG(subprocess) << "pushing bundle";

    // determine how much memory we need to allocate for this final bundle (rtp packets + payload_size * num payloads)
    size_t bundleSize = m_rtpBytesInQueue;
    
    std::vector<uint8_t> outgoingBundle(bundleSize);
    // LOG_DEBUG(subprocess) << "allocated  bundle size " << bundleSize;

    size_t offset = 0;
    
    // uint64_t count = 0;
    while (m_outgoingRtpPacketQueue.size() != 0)
    {
        // copy the packet size and rtp packet into our bundle
        memcpy(outgoingBundle.data() + offset, m_outgoingRtpPacketQueue.front().data(), m_outgoingRtpPacketQueue.front().size());
        offset +=  m_outgoingRtpPacketQueue.front().size();

        // size_t * length = (size_t * ) m_outgoingRtpPacketQueue.front().data();
        // std::cout << *length;
        // rtp_frame * frame = (rtp_frame *)  m_outgoingRtpPacketQueue.front().data() + sizeof(size_t);
        // frame->print_header();

        m_outgoingRtpPacketQueue.pop();
        // count ++;
        // LOG_DEBUG(subprocess) << "copied rtp packet " << count << " into bundle";
    }
    
    m_rtpBytesInQueue = 0;

    {
        boost::mutex::scoped_lock lock(m_outgoingQueueMutex);    // lock mutex 
        if (m_outgoingCircularBundleQueue.full())
            m_totalOutgoingCbOverruns++;
        m_outgoingCircularBundleQueue.push_back(std::move(outgoingBundle)); 
    }
    m_outgoingQueueCv.notify_one();
}

void BpSendStream::DeleteCallback()
{
}

uint64_t BpSendStream::GetNextPayloadLength_Step1() 
{
    m_outgoingQueueMutex.lock();

    if (m_outgoingCircularBundleQueue.size() == 0) {
        m_outgoingQueueMutex.unlock();

        // LOG_DEBUG(subprocess) << "Circ Queue Size: " << m_outgoingCircularBundleQueue.size() << " waiting...";
        return UINT64_MAX; // wait for data
    } 
    
    return (uint64_t) m_outgoingCircularBundleQueue.front().size();
}

bool BpSendStream::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    memcpy(destinationBuffer, m_outgoingCircularBundleQueue.front().data(), m_outgoingCircularBundleQueue.front().size());
    m_outgoingCircularBundleQueue.pop_front(); 
    m_outgoingQueueMutex.unlock();
    m_outgoingQueueCv.notify_one();
    // m_totalRtpPacketsSent++; 

    return true;
}

/**
 * If TryWaitForDataAvailable returns true, BpSourcePattern will move to export data (Step1 and Step2). 
 * If TryWaitForDataAvailable returns false, BpSourcePattern will recall this function after a timeout
*/
bool BpSendStream::TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) {
    if (m_outgoingCircularBundleQueue.size()==0) {
        return GetNextOutgoingPacketTimeout(timeout);
    }

    return true; 
}
bool BpSendStream::GetNextOutgoingPacketTimeout(const boost::posix_time::time_duration& timeout)
{
    boost::mutex::scoped_lock lock(m_outgoingQueueMutex);
    bool inWaitForPacketState = (m_outgoingCircularBundleQueue.size() == 0);

    if (inWaitForPacketState) {
        m_outgoingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }

    return true;
}



/**
 * If return true, we have data
 * If return false, we do not have data
*/
bool BpSendStream::TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout) {
    if (m_incomingCircularPacketQueue.size() == 0) { // if empty, we wait
        return GetNextIncomingPacketTimeout(timeout);
    }
    return true; 
}


bool BpSendStream::GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_incomingQueueMutex);
    if ((m_incomingCircularPacketQueue.size() == 0)) {
        m_incomingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }
    
    return true;
}

bool BpSendStream::SdpTimerThread()
{
    if (m_sdpFileStr.size() == 0)
        return true;
    
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    std::vector<uint8_t> sdpBuffer(m_sdpFileStr.size() + sizeof(uint64_t));
    uint64_t header = SDP_FILE_STR_HEADER;
    memcpy(sdpBuffer.data(), &header, sizeof(uint64_t));
    memcpy(sdpBuffer.data() + sizeof(uint64_t), m_sdpFileStr.data(), m_sdpFileStr.size());

    while (1)
    {     
        {
            boost::mutex::scoped_lock lock(m_outgoingQueueMutex);
            m_outgoingCircularBundleQueue.push_back(sdpBuffer);
            LOG_INFO(subprocess) << "Sending SDP Packet";
        }

        m_outgoingQueueCv.notify_one();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(m_sdpInterval_ms));
    }
    return false;
}




void BpSendStream::StartTcpAccept() {
    LOG_INFO(subprocess) << "waiting for  tcp connections";
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string("127.0.0.1"), 50000);

    m_tcpSocketPtr = std::make_shared<boost::asio::ip::tcp::socket>(m_ioService); //get_io_service() is deprecated: Use get_executor()
    m_tcpAcceptorPtr->async_accept(*m_tcpSocketPtr, 
            boost::bind(&BpSendStream::HandleTcpAccept, this, m_tcpSocketPtr, boost::asio::placeholders::error));
}

void BpSendStream::HandleTcpAccept(std::shared_ptr<boost::asio::ip::tcp::socket> & newTcpSocketPtr, const boost::system::error_code& error) {
    if (!error) {
        LOG_INFO(subprocess) << "tcp connection: " << newTcpSocketPtr->remote_endpoint().address() << ":" << newTcpSocketPtr->remote_endpoint().port();
        
        m_tcpPacketSinkPtr = std::make_shared<TcpPacketSink>(m_tcpSocketPtr, m_ioService, 
        boost::bind(&BpSendStream::WholeBundleReadyCallback, this, boost::placeholders::_1),
        m_numCircularBufferVectors, 
        m_maxIncomingUdpPacketSizeBytes, 
        boost::bind(&BpSendStream::DeleteCallback, this));
        // StartTcpAccept(); //only accept if there was no error
    }
    else if (error != boost::asio::error::operation_aborted) {
        LOG_ERROR(subprocess) << "tcp accept error: " << error.message();
    }


}



















void BpSendStream::InitFdSink()
{
    // m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    m_fd = make_named_socket("/tmp/mysocket5");

    if (m_fd < 0)
    {
        LOG_ERROR(subprocess) << "Could not create FD";
    }

}

void BpSendStream::FdSinkThread()
{
    // connect to socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/mysocket5", sizeof(addr.sun_path)-1);

    if (connect(m_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        exit(-1);
    }

    LOG_INFO(subprocess) << "Connected";

    int nbytes;
    char buf[2000];
    while (1)
    {

    nbytes = recvfrom (m_fd, buf, 2000, 0, NULL, 0);
    
    if (nbytes < 0)
        {
        perror ("recfrom (client)");
        exit (EXIT_FAILURE);
        }

    if (nbytes  > 0)
        FdPushToQueue(buf, nbytes);
  /* Print a diagnostic message. */
//   fprintf (stderr, "Client: got message: %s\n", buf);
//  LOG_INFO(subprocess) << "Size of message=" << nbytes;

    // read data from fd sink

    }

}

void BpSendStream::FdPushToQueue(void * buf, size_t size)
{
    padded_vector_uint8_t vec(size);
    memcpy(vec.data(), buf, size);

    {
        boost::mutex::scoped_lock lock(m_incomingQueueMutex);// lock mutex 
        // LOG_DEBUG(subprocess) << "Pushing frame into incoming queue";
        if (m_incomingCircularPacketQueue.full())
            m_totalIncomingCbOverruns++;

        m_incomingCircularPacketQueue.push_back(std::move(vec));  // copy out bundle to local queue for processing
        // rtp_header * header = (rtp_header *) wholeBundleVec.data();
    }
    m_incomingQueueCv.notify_one();
}

void BpSendStream::ExecuteGst(std::string gstCommand)
{
    gstCommand = "gst-launch-1.0 filesrc location=/home/kyle/nasa/dev/test_media/official_test_media/lucia_crf18_g_15.mp4 ! qtdemux ! h264parse ! rtph264pay config-interval=4 ! fdsink max-bitrate=10000 sync=true fd=";
    gstCommand.append(std::to_string(m_fd));


    LOG_INFO(subprocess) << "GST Command:\n" << gstCommand;

    boost::this_thread::sleep_for(boost::chrono::microseconds(3000));

    boost::process::child gstProcess(gstCommand);
    gstProcess.detach();
}