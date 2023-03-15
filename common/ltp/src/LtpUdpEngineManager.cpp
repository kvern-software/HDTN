/**
 * @file LtpUdpEngineManager.cpp
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright © 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 */

#include "LtpUdpEngineManager.h"
#include "Logger.h"
#include <boost/make_unique.hpp>
#include <boost/lexical_cast.hpp>
#include "Sdnv.h"
#include "ThreadNamer.h"

//c++ shared singleton using weak pointer
//https://codereview.stackexchange.com/questions/14343/c-shared-singleton

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

std::map<uint16_t, std::weak_ptr<LtpUdpEngineManager> > LtpUdpEngineManager::m_staticMapBoundPortToLtpUdpEngineManagerPtr;
boost::mutex LtpUdpEngineManager::m_staticMutex;
uint64_t LtpUdpEngineManager::M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES = 0;

//static function
void LtpUdpEngineManager::SetMaxUdpRxPacketSizeBytesForAllLtp(const uint64_t maxUdpRxPacketSizeBytesForAllLtp) {
    boost::mutex::scoped_lock theLock(m_staticMutex);
    if (maxUdpRxPacketSizeBytesForAllLtp == 0) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::SetMaxUdpRxPacketSizeBytesForAllLtp: LTP Max RX UDP packet size cannot be zero";
    }
    else if (maxUdpRxPacketSizeBytesForAllLtp < 100) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::SetMaxUdpRxPacketSizeBytesForAllLtp: LTP Max RX UDP packet size must be at least 100 bytes";
    }
    else if (M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES == 0) {
        M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES = maxUdpRxPacketSizeBytesForAllLtp;
        LOG_INFO(subprocess) << "All LTP UDP engines can receive a maximum of " << M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES << " bytes per packet";
    }
    else if (M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES != maxUdpRxPacketSizeBytesForAllLtp) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::SetMaxUdpRxPacketSizeBytesForAllLtp: LTP Max RX UDP packet size cannot be changed from " 
            << M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES << " to " << maxUdpRxPacketSizeBytesForAllLtp;
    }
}

//static function
std::shared_ptr<LtpUdpEngineManager> LtpUdpEngineManager::GetOrCreateInstance(const uint16_t myBoundUdpPort, const bool autoStart) {
    boost::mutex::scoped_lock theLock(m_staticMutex);
    std::shared_ptr<LtpUdpEngineManager> sp;
    if (M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES == 0) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::GetOrCreateInstance: LTP Max RX UDP packet size not set.. call SetMaxUdpRxPacketSizeBytesForAllLtp() first";
    }
    else {
        std::map<uint16_t, std::weak_ptr<LtpUdpEngineManager> >::iterator it = m_staticMapBoundPortToLtpUdpEngineManagerPtr.find(myBoundUdpPort);
        if ((it == m_staticMapBoundPortToLtpUdpEngineManagerPtr.end()) || (it->second.expired())) { //create new instance
            sp.reset(new LtpUdpEngineManager(myBoundUdpPort, autoStart));
            m_staticMapBoundPortToLtpUdpEngineManagerPtr[myBoundUdpPort] = sp;
        }
        else {
            sp = it->second.lock();
        }
    }
    return sp;
}

//private constructor
LtpUdpEngineManager::LtpUdpEngineManager(const uint16_t myBoundUdpPort, const bool autoStart) :
    M_MY_BOUND_UDP_PORT(myBoundUdpPort),
    m_resolver(m_ioServiceUdp),
    m_udpSocket(m_ioServiceUdp),
    m_retryAfterSocketErrorTimer(m_ioServiceUdp),
    m_socketRestoredTimer(m_ioServiceUdp),
    m_udpReceiveBuffer(M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES),
    m_cachedItRemoteEngineIdToLtpUdpEngineReceiver(m_mapRemoteEngineIdToLtpUdpEngineReceiver.end()),
    m_vecEngineIndexToLtpUdpEngineTransmitterPtr(256, NULL),
    m_nextEngineIndex(1),
    m_readyToForward(false)
{
    if (autoStart) {
        StartIfNotAlreadyRunning(); //TODO EVALUATE IF AUTO START SAFE
    }
}

//call Start after all UDP engines have been added
bool LtpUdpEngineManager::StartIfNotAlreadyRunning() {
    if (!m_ioServiceUdpThreadPtr) {
        //Receiver UDP
        try {
            m_udpSocket.open(boost::asio::ip::udp::v4());
            m_udpSocket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), M_MY_BOUND_UDP_PORT)); // //if udpPort is 0 then bind to random ephemeral port
        }
        catch (const boost::system::system_error & e) {
            LOG_ERROR(subprocess) << "Could not bind on UDP port " << M_MY_BOUND_UDP_PORT;
            LOG_ERROR(subprocess) << e.what();

            return false;
        }
        LOG_INFO(subprocess) << "LtpUdpEngineManager bound successfully on UDP port " << m_udpSocket.local_endpoint().port();

        StartUdpReceive(); //call before creating io_service thread so that it has "work"

        m_ioServiceUdpThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_ioServiceUdp));
        const std::string threadName = "ioServiceLtpUdpSocket port=" + boost::lexical_cast<std::string>(M_MY_BOUND_UDP_PORT);
        ThreadNamer::SetIoServiceThreadName(m_ioServiceUdp, threadName);
        m_readyToForward = true;
    }
    return true;
}

LtpUdpEngine * LtpUdpEngineManager::GetLtpUdpEnginePtrByRemoteEngineId(const uint64_t remoteEngineId, const bool isInduct) {
    std::map<uint64_t, LtpUdpEngine> * const whichMap = (isInduct) ? &m_mapRemoteEngineIdToLtpUdpEngineReceiver : &m_mapRemoteEngineIdToLtpUdpEngineTransmitter;
    std::map<uint64_t, LtpUdpEngine>::iterator it = whichMap->find(remoteEngineId);
    return (it == whichMap->end()) ? NULL : &(it->second);
}

void LtpUdpEngineManager::RemoveLtpUdpEngineByRemoteEngineId_ThreadSafe(const uint64_t remoteEngineId, const bool isInduct, const boost::function<void()> & callback) {
    boost::asio::post(m_ioServiceUdp, boost::bind(&LtpUdpEngineManager::RemoveLtpUdpEngineByRemoteEngineId_NotThreadSafe, this, remoteEngineId, isInduct, callback));
}
void LtpUdpEngineManager::RemoveLtpUdpEngineByRemoteEngineId_NotThreadSafe(const uint64_t remoteEngineId, const bool isInduct, const boost::function<void()> & callback) {
    std::map<uint64_t, LtpUdpEngine> * const whichMap = (isInduct) ? &m_mapRemoteEngineIdToLtpUdpEngineReceiver : &m_mapRemoteEngineIdToLtpUdpEngineTransmitter;
    std::map<uint64_t, LtpUdpEngine>::iterator it = whichMap->find(remoteEngineId);
    if (it == whichMap->end()) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::RemoveLtpUdpEngineByRemoteEngineId_NotThreadSafe: remoteEngineId " << remoteEngineId
            << " for type " << ((isInduct) ? "induct" : "outduct") << " does not exist";
    }
    else {
        if (!isInduct) {
            const uint8_t engineIndex = it->second.GetEngineIndex();
            if (m_vecEngineIndexToLtpUdpEngineTransmitterPtr[engineIndex]) {
                m_vecEngineIndexToLtpUdpEngineTransmitterPtr[engineIndex] = NULL;
                //TODO add engineIndex back to available engineIndices in case of adding/removing/readding
            }
            else {
                LOG_ERROR(subprocess) << "LtpUdpEngineManager::RemoveLtpUdpEngineByRemoteEngineId_NotThreadSafe: remoteEngineId " << remoteEngineId
                    << " for type outduct had non-null m_vecEngineIndexToLtpUdpEngineTransmitterPtr at engineIndex " << ((unsigned int)engineIndex);
            }
        }
        else { //induct
            if (m_cachedItRemoteEngineIdToLtpUdpEngineReceiver == it) {
                m_cachedItRemoteEngineIdToLtpUdpEngineReceiver = m_mapRemoteEngineIdToLtpUdpEngineReceiver.end();
            }
        }
        whichMap->erase(it);
        LOG_INFO(subprocess) << "remoteEngineId " << remoteEngineId << " for type " << ((isInduct) ? "induct" : "outduct") << " successfully removed";
    }
    if (callback) {
        callback();
    }
}

//a max of 7 engines can be added for one outduct with the same udp port (IDs 1,2,3,4,5,6,7)
bool LtpUdpEngineManager::AddLtpUdpEngine(const LtpEngineConfig& ltpRxOrTxCfg)
{   
    const bool isInduct = ltpRxOrTxCfg.isInduct;
    const bool isOutduct = (!isInduct);
    if ((ltpRxOrTxCfg.delaySendingOfReportSegmentsTimeMsOrZeroToDisable != 0) && isOutduct) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: delaySendingOfReportSegmentsTimeMsOrZeroToDisable must be set to 0 for an outduct";
        return false;
    }
    if ((ltpRxOrTxCfg.delaySendingOfDataSegmentsTimeMsOrZeroToDisable != 0) && isInduct) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: delaySendingOfDataSegmentsTimeMsOrZeroToDisable must be set to 0 for an induct";
        return false;
    }
    if ((m_nextEngineIndex > 7) && isOutduct) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: a max of 7 engines can be added for one outduct with the same udp port";
        return false;
    }
    if (ltpRxOrTxCfg.maxUdpPacketsToSendPerSystemCall == 0) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: maxUdpPacketsToSendPerSystemCall must be non-zero.";
        return false;
    }
#ifdef UIO_MAXIOV
    //sendmmsg() is Linux-specific. NOTES The value specified in vlen is capped to UIO_MAXIOV (1024).
    if (ltpRxOrTxCfg.maxUdpPacketsToSendPerSystemCall > UIO_MAXIOV) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: maxUdpPacketsToSendPerSystemCall ("
            << ltpRxOrTxCfg.maxUdpPacketsToSendPerSystemCall << ") must be <= UIO_MAXIOV (" << UIO_MAXIOV << ").";
        return false;
    }
#endif //UIO_MAXIOV
    if (ltpRxOrTxCfg.senderPingSecondsOrZeroToDisable && isInduct) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: senderPingSecondsOrZeroToDisable cannot be used with an induct (must be set to 0).";
        return false;
    }
    if (ltpRxOrTxCfg.activeSessionDataOnDiskNewFileDurationMsOrZeroToDisable != 0) { //if enabled
        if (ltpRxOrTxCfg.activeSessionDataOnDiskNewFileDurationMsOrZeroToDisable < 1000) {
            LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: activeSessionDataOnDiskNewFileDurationMsOrZeroToDisable, if non-zero, must be 1000ms minimum.";
            return false;
        }
        else if (ltpRxOrTxCfg.activeSessionDataOnDiskDirectory.empty()) {
            LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: activeSessionDataOnDiskDirectory must be defined when activeSessionDataOnDisk feature is enabled.";
            return false;
        }
        else if (ltpRxOrTxCfg.maxSimultaneousSessions < 8) { //because of "static constexpr uint64_t diskPipelineLimit = 6;" in LtpEngine.cpp, want a number greater than 6
            LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: when activeSessionDataOnDisk feature is enabled, maxSimultaneousSessions must be at least 8.";
            return false;
        }
    }
    std::map<uint64_t, LtpUdpEngine> * const whichMap = (isInduct) ? &m_mapRemoteEngineIdToLtpUdpEngineReceiver : &m_mapRemoteEngineIdToLtpUdpEngineTransmitter;
    std::map<uint64_t, LtpUdpEngine>::iterator it = whichMap->find(ltpRxOrTxCfg.remoteEngineId);
    if (it != whichMap->end()) {
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: remote engine Id " << ltpRxOrTxCfg.remoteEngineId
            << " for type " << ((isInduct) ? "induct" : "outduct") << " already exists";
        return false;
    }

    static const boost::asio::ip::resolver_query_base::flags UDP_RESOLVER_FLAGS = boost::asio::ip::resolver_query_base::canonical_name; //boost resolver flags
    boost::asio::ip::udp::endpoint remoteEndpoint;
    try {
        remoteEndpoint = *m_resolver.resolve(boost::asio::ip::udp::resolver::query(boost::asio::ip::udp::v4(),
            ltpRxOrTxCfg.remoteHostname, boost::lexical_cast<std::string>(ltpRxOrTxCfg.remotePort), UDP_RESOLVER_FLAGS));
    }
    catch (const boost::system::system_error & e) {
        LOG_INFO(subprocess) << "LtpUdpEngineManager::AddLtpUdpEngine: " << e.what() << "  code=" << e.code();
        return false;
    }
    LOG_INFO(subprocess) << "Adding LTP engineId: " << ltpRxOrTxCfg.thisEngineId 
        << " who will talk with remote " <<  remoteEndpoint.address() << ":" << remoteEndpoint.port();

    const uint8_t engineIndex = static_cast<uint8_t>(m_nextEngineIndex); //this is a don't care for inducts, only needed for outducts

    std::pair<std::map<uint64_t, LtpUdpEngine>::iterator, bool> res = whichMap->emplace(
        std::piecewise_construct,
        std::forward_as_tuple(ltpRxOrTxCfg.remoteEngineId),
        std::forward_as_tuple(
            m_ioServiceUdp, m_udpSocket, engineIndex, remoteEndpoint,
            M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES, ltpRxOrTxCfg
        )
    );
    if (res.second == false) { //session was not inserted
        LOG_ERROR(subprocess) << "new LtpUdpEngine cannot be inserted??";
        return false;
    }

    if (isOutduct) {
        ++m_nextEngineIndex;
        m_vecEngineIndexToLtpUdpEngineTransmitterPtr[engineIndex] = &(res.first->second);
    }
    
    return true;
}

LtpUdpEngineManager::~LtpUdpEngineManager() {
    Stop();
    LOG_DEBUG(subprocess) << "~LtpUdpEngineManager";
}

void LtpUdpEngineManager::Stop() {
    if (m_ioServiceUdpThreadPtr) {
        boost::asio::post(m_ioServiceUdp, boost::bind(&LtpUdpEngineManager::DoUdpShutdown, this));
        try {
            m_ioServiceUdpThreadPtr->join();
            m_ioServiceUdpThreadPtr.reset(); //delete it
        }
        catch (const boost::thread_resource_error&) {
            LOG_ERROR(subprocess) << "error stopping LtpUdpEngineManager io_service udp thread";
        }
    }

    //print stats
    
}


void LtpUdpEngineManager::StartUdpReceive() {
    m_udpSocket.async_receive_from(
        boost::asio::buffer(m_udpReceiveBuffer),
        m_remoteEndpointReceived,
        boost::bind(&LtpUdpEngineManager::HandleUdpReceive, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}


void LtpUdpEngineManager::HandleUdpReceive(const boost::system::error_code & error, std::size_t bytesTransferred) {
    if (!error) {
        if (bytesTransferred <= 2) {
            LOG_ERROR(subprocess) << "LtpUdpEngineManager::HandleUdpReceive(): bytesTransferred <= 2 .. ignoring packet";
            StartUdpReceive();
            return;
        }
        
        const uint8_t segmentTypeFlags = m_udpReceiveBuffer[0]; // & 0x0f; //upper 4 bits must be 0 for version 0
        bool isSenderToReceiver;
        if (!Ltp::GetMessageDirectionFromSegmentFlags(segmentTypeFlags, isSenderToReceiver)) {
            LOG_FATAL(subprocess) << "LtpUdpEngineManager::HandleUdpReceive(): received invalid ltp packet with segment type flag " << (int)segmentTypeFlags;
            DoUdpShutdown();
            return;
        }
#if defined(USE_SDNV_FAST) && defined(SDNV_SUPPORT_AVX2_FUNCTIONS)
        uint64_t decodedValues[2];
        const unsigned int numSdnvsToDecode = 2u - isSenderToReceiver;
        uint8_t totalBytesDecoded;
        unsigned int numValsDecodedThisIteration = SdnvDecodeMultiple256BitU64Fast(&m_udpReceiveBuffer[1], &totalBytesDecoded, decodedValues, numSdnvsToDecode);
        if (numValsDecodedThisIteration != numSdnvsToDecode) { //all required sdnvs were not decoded, possibly due to a decode error
            LOG_ERROR(subprocess) << "LtpUdpEngineManager::HandleUdpReceive(): cannot read 1 or more of sessionOriginatorEngineId or sessionNumber.. ignoring packet";
            StartUdpReceive();
            return;
        }
        const uint64_t & sessionOriginatorEngineId = decodedValues[0];
        const uint64_t & sessionNumber = decodedValues[1];
#else
        uint8_t sdnvSize;
        const uint64_t sessionOriginatorEngineId = SdnvDecodeU64(&m_udpReceiveBuffer[1], &sdnvSize, (100 - 1)); //no worries about hardware accelerated sdnv read out of bounds due to minimum 100 byte size
        if (sdnvSize == 0) {
            LOG_ERROR(subprocess) << "LtpUdpEngineManager::HandleUdpReceive(): cannot read sessionOriginatorEngineId.. ignoring packet";
            StartUdpReceive();
            return;
        }
        uint64_t sessionNumber;
        if (!isSenderToReceiver) {
            sessionNumber = SdnvDecodeU64(&m_udpReceiveBuffer[1 + sdnvSize], &sdnvSize, ((100 - 10) - 1)); //no worries about hardware accelerated sdnv read out of bounds due to minimum 100 byte size
            if (sdnvSize == 0) {
                LOG_ERROR(subprocess) << "LtpUdpEngineManager::HandleUdpReceive(): cannot read sessionNumber.. ignoring packet";
                StartUdpReceive();
                return;
            }
        }
#endif

        LtpUdpEngine * ltpUdpEnginePtr;
        if (isSenderToReceiver) { //received an isSenderToReceiver message type => isInduct (this ltp engine received a message type that only travels from an outduct (sender) to an induct (receiver))
            //sessionOriginatorEngineId is the remote engine id in the case of an induct
            std::map<uint64_t, LtpUdpEngine>::iterator it = m_cachedItRemoteEngineIdToLtpUdpEngineReceiver;
            if ((it == m_mapRemoteEngineIdToLtpUdpEngineReceiver.end()) || (it->first != sessionOriginatorEngineId)) { //cache miss (invalid OR non-match)
                it = m_mapRemoteEngineIdToLtpUdpEngineReceiver.find(sessionOriginatorEngineId);
                if (it == m_mapRemoteEngineIdToLtpUdpEngineReceiver.end()) {
                    LOG_ERROR(subprocess) << "LtpUdpEngineManager::HandleUdpReceive: an induct received packet with unknown remote engine Id "
                        << sessionOriginatorEngineId << ".. ignoring packet";
                    StartUdpReceive();
                    return;
                }
                m_cachedItRemoteEngineIdToLtpUdpEngineReceiver = it; //cache update
            }
            ltpUdpEnginePtr = &(it->second);
        }
        else { //received an isReceiverToSender message type => isOutduct (this ltp engine received a message type that only travels from an induct (receiver) to an outduct (sender))
            //sessionOriginatorEngineId is my engine id in the case of an outduct.. need to get the session number to find the proper LtpUdpEngine
            const uint8_t engineIndex = LtpRandomNumberGenerator::GetEngineIndexFromRandomSessionNumber(sessionNumber);
            ltpUdpEnginePtr = m_vecEngineIndexToLtpUdpEngineTransmitterPtr[engineIndex];
            if (ltpUdpEnginePtr == NULL) {
                LOG_ERROR(subprocess) << "LtpUdpEngineManager::HandleUdpReceive: an outduct received packet of type " << (int)segmentTypeFlags << " with unknown session number "
                    << sessionNumber << ".. ignoring packet";
                StartUdpReceive();
                return;
            }
        }
        
        ltpUdpEnginePtr->PostPacketFromManager_ThreadSafe(m_udpReceiveBuffer, bytesTransferred);
        if (m_udpReceiveBuffer.size() != M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES) {
            LOG_ERROR(subprocess) << "LtpUdpEngineManager::HandleUdpReceive: swapped packet not size " 
                << M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES << "... resizing";
            m_udpReceiveBuffer.resize(M_STATIC_MAX_UDP_RX_PACKET_SIZE_BYTES_FOR_ALL_LTP_UDP_ENGINES);
        }
        StartUdpReceive(); //restart operation only if there was no error
    }
    else if (error != boost::asio::error::operation_aborted) {
        //this happens with windows loopback (localhost) peer udp sockets being terminated
        m_readyToForward = false;
        LOG_ERROR(subprocess) << "LtpUdpEngineManager::HandleUdpReceive(): " << error.message() << std::endl
            << "Will try to Receive after 2 seconds";
        for (std::map<uint64_t, LtpUdpEngine>::iterator it = m_mapRemoteEngineIdToLtpUdpEngineTransmitter.begin();
            it != m_mapRemoteEngineIdToLtpUdpEngineTransmitter.end(); ++it)
        {
            it->second.PostExternalLinkDownEvent_ThreadSafe();
        }
        m_socketRestoredTimer.cancel();
        m_retryAfterSocketErrorTimer.expires_from_now(boost::posix_time::seconds(2));
        m_retryAfterSocketErrorTimer.async_wait(boost::bind(&LtpUdpEngineManager::OnRetryAfterSocketError_TimerExpired, this, boost::asio::placeholders::error));
        //DoUdpShutdown();
    }
}

void LtpUdpEngineManager::OnRetryAfterSocketError_TimerExpired(const boost::system::error_code& e) {
    if (e != boost::asio::error::operation_aborted) {
        // Timer was not cancelled, take necessary action.
        LOG_INFO(subprocess) << "Trying to receive...";
        m_socketRestoredTimer.expires_from_now(boost::posix_time::seconds(5));
        m_socketRestoredTimer.async_wait(boost::bind(&LtpUdpEngineManager::SocketRestored_TimerExpired, this, boost::asio::placeholders::error));
        StartUdpReceive();
    }
}

void LtpUdpEngineManager::SocketRestored_TimerExpired(const boost::system::error_code& e) {
    if (e != boost::asio::error::operation_aborted) {
        // Timer was not cancelled, take necessary action.
        LOG_INFO(subprocess) << "LtpUdpEngineManager socket back to receiving";
        m_readyToForward = true;
    }
}




void LtpUdpEngineManager::DoUdpShutdown() {
    //final code to shut down tcp sockets
    m_readyToForward = false;
    try {
        m_retryAfterSocketErrorTimer.cancel();
    }
    catch (const boost::system::system_error& e) {
        LOG_WARNING(subprocess) << "LtpUdpEngineManager::DoUdpShutdown calling retryAfterSocketErrorTimer.cancel(): " << e.what();
    }
    try {
        m_socketRestoredTimer.cancel();
    }
    catch (const boost::system::system_error& e) {
        LOG_WARNING(subprocess) << "LtpUdpEngineManager::DoUdpShutdown calling socketRestoredTimer.cancel(): " << e.what();
    }
    if (m_udpSocket.is_open()) {
        try {
            LOG_INFO(subprocess) << "closing LtpUdpEngineManager UDP socket..";
            m_udpSocket.close();
        }
        catch (const boost::system::system_error & e) {
            LOG_WARNING(subprocess) << "LtpUdpEngineManager::DoUdpShutdown calling udpSocket.close: " << e.what();
        }
    }
    m_mapRemoteEngineIdToLtpUdpEngineReceiver.clear();
    m_cachedItRemoteEngineIdToLtpUdpEngineReceiver = m_mapRemoteEngineIdToLtpUdpEngineReceiver.end();
    m_mapRemoteEngineIdToLtpUdpEngineTransmitter.clear();
}

bool LtpUdpEngineManager::ReadyToForward() {
    return m_readyToForward;
}
