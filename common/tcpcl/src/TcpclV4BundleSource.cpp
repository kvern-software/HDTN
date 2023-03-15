/**
 * @file TcpclV4BundleSource.cpp
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

#include <string>
#include "TcpclV4BundleSource.h"
#include "Logger.h"
#include <boost/lexical_cast.hpp>
#include <memory>
#include <boost/make_unique.hpp>
#include "Uri.h"
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

TcpclV4BundleSource::TcpclV4BundleSource(
#ifdef OPENSSL_SUPPORT_ENABLED
    boost::asio::ssl::context & shareableSslContextRef,
#endif
    const bool tryUseTls, const bool tlsIsRequired,
    const uint16_t desiredKeepAliveIntervalSeconds, const uint64_t myNodeId,
    const std::string & expectedRemoteEidUri, const unsigned int maxUnacked, const uint64_t myMaxRxSegmentSizeBytes, const uint64_t myMaxRxBundleSizeBytes,
    const OutductOpportunisticProcessReceivedBundleCallback_t & outductOpportunisticProcessReceivedBundleCallback) :

    TcpclV4BidirectionalLink(
        "TcpclV4BundleSource",
        0, //shutdown message shall send 0 meaning infinite reconnection delay (sink shall not try to reconnect)
        false, //bundleSource shall NOT delete socket after shutdown
        true, //isActiveEntity,
        desiredKeepAliveIntervalSeconds,
        NULL, // NULL will create a local io_service
        maxUnacked,
        myMaxRxSegmentSizeBytes,
        myMaxRxBundleSizeBytes, //100000000, //todo 100MB maxBundleSizeBytes for receive
        myNodeId,
        expectedRemoteEidUri,
#ifdef OPENSSL_SUPPORT_ENABLED
        tryUseTls, //tryUseTls
#else
        false,
#endif
        tlsIsRequired //tlsIsRequired
    ),
#ifdef OPENSSL_SUPPORT_ENABLED
    m_shareableSslContextRef(shareableSslContextRef),
#endif
    m_work(m_base_ioServiceRef), //prevent stopping of ioservice until destructor
    m_resolver(m_base_ioServiceRef),
    m_reconnectAfterShutdownTimer(m_base_ioServiceRef),
    m_reconnectAfterOnConnectErrorTimer(m_base_ioServiceRef),
    m_outductOpportunisticProcessReceivedBundleCallback(outductOpportunisticProcessReceivedBundleCallback),
    m_tcpReadSomeBufferVec(10000) //todo 10KB rx buffer
{
    m_ioServiceThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_base_ioServiceRef));
    ThreadNamer::SetIoServiceThreadName(m_base_ioServiceRef, "ioServiceTcpclV4BundleSource");
}

TcpclV4BundleSource::~TcpclV4BundleSource() {
    Stop();
    //print stats
    LOG_INFO(subprocess) << "TcpclV4 Bundle Source totalBundlesAcked " << m_base_outductTelemetry.m_totalBundlesAcked;
    LOG_INFO(subprocess) << "TcpclV4 Bundle Source totalBundleBytesAcked " << m_base_outductTelemetry.m_totalBundleBytesAcked;
    LOG_INFO(subprocess) << "TcpclV4 Bundle Source totalBundlesSent " << m_base_outductTelemetry.m_totalBundlesSent;
    LOG_INFO(subprocess) << "TcpclV4 Bundle Source totalFragmentsAcked " << m_base_outductTelemetry.m_totalFragmentsAcked;
    LOG_INFO(subprocess) << "TcpclV4 Bundle Source totalFragmentsSent " << m_base_outductTelemetry.m_totalFragmentsSent;
    LOG_INFO(subprocess) << "TcpclV4 Bundle Source totalBundleBytesSent " << m_base_outductTelemetry.m_totalBundleBytesSent;
}

void TcpclV4BundleSource::Stop() {
    //prevent TcpclBundleSource from exiting before all bundles sent and acked
    BaseClass_TryToWaitForAllBundlesToFinishSending();

    BaseClass_DoTcpclShutdown(true, TCPCLV4_SESSION_TERMINATION_REASON_CODES::UNKNOWN, false);
    while (!m_base_tcpclShutdownComplete) {
        try {
            boost::this_thread::sleep(boost::posix_time::milliseconds(250));
        }
        catch (const boost::thread_resource_error&) {}
        catch (const boost::thread_interrupted&) {}
        catch (const boost::condition_error&) {}
        catch (const boost::lock_error&) {}
    }
#ifdef OPENSSL_SUPPORT_ENABLED
    m_base_tcpAsyncSenderSslPtr.reset(); //stop this first
#else
    m_base_tcpAsyncSenderPtr.reset(); //stop this first
#endif
    m_base_ioServiceRef.stop(); //ioservice requires stopping before join because of the m_work object

    if (m_ioServiceThreadPtr) {
        try {
            m_ioServiceThreadPtr->join();
            m_ioServiceThreadPtr.reset(); //delete it
        }
        catch (const boost::thread_resource_error&) {
            LOG_ERROR(subprocess) << "error stopping TcpclV4BundleSource io_service";
        }
    }
}



void TcpclV4BundleSource::Connect(const std::string & hostname, const std::string & port) {

    boost::asio::ip::tcp::resolver::query query(hostname, port);
    m_resolver.async_resolve(query, boost::bind(&TcpclV4BundleSource::OnResolve,
                                                this, boost::asio::placeholders::error,
                                                boost::asio::placeholders::results));
}

void TcpclV4BundleSource::OnResolve(const boost::system::error_code & ec, boost::asio::ip::tcp::resolver::results_type results) { // Resolved endpoints as a range.
    if(ec) {
        LOG_ERROR(subprocess) << "Error resolving: " << ec.message();
    }
    else {
        LOG_INFO(subprocess) << "resolved host to " << results->endpoint().address() << ":" << results->endpoint().port() << ".  Connecting...";
        m_resolverResults = results;
#ifdef OPENSSL_SUPPORT_ENABLED
        m_base_sslStreamSharedPtr = std::make_shared<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> >(m_base_ioServiceRef, m_shareableSslContextRef);
        boost::asio::async_connect(
            m_base_sslStreamSharedPtr->next_layer(),
#else
        m_base_tcpSocketPtr = std::make_shared<boost::asio::ip::tcp::socket>(m_base_ioServiceRef);
        boost::asio::async_connect(
            *m_base_tcpSocketPtr,
#endif
            m_resolverResults,
            boost::bind(
                &TcpclV4BundleSource::OnConnect,
                this,
                boost::asio::placeholders::error));
    }
}

void TcpclV4BundleSource::OnConnect(const boost::system::error_code & ec) {

    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            if (m_base_outductTelemetry.m_numTcpReconnectAttempts <= 1) {
                LOG_ERROR(subprocess) << "OnConnect: " << ec.value() << " " << ec.message();
                LOG_ERROR(subprocess) << "Will continue to try to reconnect every 2 seconds";
            }
            m_reconnectAfterOnConnectErrorTimer.expires_from_now(boost::posix_time::seconds(2));
            m_reconnectAfterOnConnectErrorTimer.async_wait(boost::bind(&TcpclV4BundleSource::OnReconnectAfterOnConnectError_TimerExpired, this, boost::asio::placeholders::error));
        }
        return;
    }

    LOG_INFO(subprocess) << "connected.. sending contact header..";
    m_base_tcpclShutdownComplete = false;

#ifdef OPENSSL_SUPPORT_ENABLED
    if (m_base_sslStreamSharedPtr) {
        m_base_tcpAsyncSenderSslPtr = boost::make_unique<TcpAsyncSenderSsl>(m_base_sslStreamSharedPtr, m_base_ioServiceRef);
        m_base_tcpAsyncSenderSslPtr->SetOnFailedBundleVecSendCallback(m_base_onFailedBundleVecSendCallback);
        m_base_tcpAsyncSenderSslPtr->SetOnFailedBundleZmqSendCallback(m_base_onFailedBundleZmqSendCallback);
        m_base_tcpAsyncSenderSslPtr->SetUserAssignedUuid(m_base_userAssignedUuid);
#else
    if (m_base_tcpSocketPtr) {
        m_base_tcpAsyncSenderPtr = boost::make_unique<TcpAsyncSender>(m_base_tcpSocketPtr, m_base_ioServiceRef);
        m_base_tcpAsyncSenderPtr->SetOnFailedBundleVecSendCallback(m_base_onFailedBundleVecSendCallback);
        m_base_tcpAsyncSenderPtr->SetOnFailedBundleZmqSendCallback(m_base_onFailedBundleZmqSendCallback);
        m_base_tcpAsyncSenderPtr->SetUserAssignedUuid(m_base_userAssignedUuid);
#endif
        BaseClass_SendContactHeader(); //(contact headers are sent without tls)
        StartTcpReceiveUnsecure();
    }
}

void TcpclV4BundleSource::OnReconnectAfterOnConnectError_TimerExpired(const boost::system::error_code& e) {
    if (e != boost::asio::error::operation_aborted) {
        // Timer was not cancelled, take necessary action.
        if (m_base_outductTelemetry.m_numTcpReconnectAttempts == 0) {
            LOG_INFO(subprocess) << "TcpclV4BundleSource Trying to reconnect...";
        }
        ++m_base_outductTelemetry.m_numTcpReconnectAttempts;

        boost::asio::async_connect(
#ifdef OPENSSL_SUPPORT_ENABLED
            m_base_sslStreamSharedPtr->next_layer(),
#else
            *m_base_tcpSocketPtr,
#endif
            m_resolverResults,
            boost::bind(
                &TcpclV4BundleSource::OnConnect,
                this,
                boost::asio::placeholders::error));
    }
}




void TcpclV4BundleSource::StartTcpReceiveUnsecure() {
#ifdef OPENSSL_SUPPORT_ENABLED
    m_base_sslStreamSharedPtr->next_layer().async_read_some(
#else
    m_base_tcpSocketPtr->async_read_some(
#endif
        boost::asio::buffer(m_tcpReadSomeBufferVec),
        boost::bind(&TcpclV4BundleSource::HandleTcpReceiveSomeUnsecure, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}
void TcpclV4BundleSource::HandleTcpReceiveSomeUnsecure(const boost::system::error_code & error, std::size_t bytesTransferred) {
    if (!error) {

        //because TcpclBundleSource will not receive much data from the destination,
        //a separate thread is not needed to process it, but rather this
        //io_service thread will do the processing
        m_base_dataReceivedServedAsKeepaliveReceived = true;
        m_base_tcpclV4RxStateMachine.HandleReceivedChars(m_tcpReadSomeBufferVec.data(), bytesTransferred);
#ifdef OPENSSL_SUPPORT_ENABLED
        if (m_base_doUpgradeSocketToSsl) { //the tcpclv4 rx state machine may have set m_base_doUpgradeSocketToSsl to true after HandleReceivedChars()
            m_base_doUpgradeSocketToSsl = false;
            LOG_INFO(subprocess) << "source calling client handshake";
            m_base_sslStreamSharedPtr->async_handshake(boost::asio::ssl::stream_base::client,
                boost::bind(&TcpclV4BundleSource::HandleSslHandshake, this, boost::asio::placeholders::error));
        }
        else
#endif
        {
            StartTcpReceiveUnsecure(); //restart operation only if there was no error
        }
    }
    else if (error == boost::asio::error::eof) {
        LOG_INFO(subprocess) << "Tcp connection closed cleanly by peer";
        BaseClass_DoTcpclShutdown(false, TCPCLV4_SESSION_TERMINATION_REASON_CODES::UNKNOWN, false);
    }
    else if (error != boost::asio::error::operation_aborted) { //will always be operation_aborted when thread is terminating
        LOG_ERROR(subprocess) << "TcpclV4BundleSource::HandleTcpReceiveSomeUnsecure: " << error.message();
        BaseClass_DoTcpclShutdown(false, TCPCLV4_SESSION_TERMINATION_REASON_CODES::UNKNOWN, false);
    }
}

#ifdef OPENSSL_SUPPORT_ENABLED
void TcpclV4BundleSource::StartTcpReceiveSecure() {
    m_base_sslStreamSharedPtr->async_read_some(
        boost::asio::buffer(m_tcpReadSomeBufferVec),
        boost::bind(&TcpclV4BundleSource::HandleTcpReceiveSomeSecure, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}
void TcpclV4BundleSource::HandleTcpReceiveSomeSecure(const boost::system::error_code & error, std::size_t bytesTransferred) {
    if (!error) {
        //because TcpclBundleSource will not receive much data from the destination,
        //a separate thread is not needed to process it, but rather this
        //io_service thread will do the processing
        m_base_dataReceivedServedAsKeepaliveReceived = true;
        m_base_tcpclV4RxStateMachine.HandleReceivedChars(m_tcpReadSomeBufferVec.data(), bytesTransferred);
        StartTcpReceiveSecure(); //restart operation only if there was no error
    }
    else if (error == boost::asio::error::eof) {
        LOG_INFO(subprocess) << "Tcp connection closed cleanly by peer";
        BaseClass_DoTcpclShutdown(false, TCPCLV4_SESSION_TERMINATION_REASON_CODES::UNKNOWN, false);
    }
    else if (error != boost::asio::error::operation_aborted) { //will always be operation_aborted when thread is terminating
        LOG_ERROR(subprocess) << "TcpclV4BundleSource::HandleTcpReceiveSomeSecure: " << error.message();
        BaseClass_DoTcpclShutdown(false, TCPCLV4_SESSION_TERMINATION_REASON_CODES::UNKNOWN, false);
    }
}

void TcpclV4BundleSource::HandleSslHandshake(const boost::system::error_code & error) {
    if (!error) {
        LOG_INFO(subprocess) << "SSL/TLS Handshake succeeded.. all transmissions shall be secure from this point";
        m_base_didSuccessfulSslHandshake = true;
        StartTcpReceiveSecure();
        BaseClass_SendSessionInit(); //I am the active entity and will send a session init first
    }
    else {
        LOG_ERROR(subprocess) << "SSL/TLS Handshake failed: " << error.message();
        BaseClass_DoTcpclShutdown(false, TCPCLV4_SESSION_TERMINATION_REASON_CODES::UNKNOWN, false);
    }
}
#endif //OPENSSL_SUPPORT_ENABLED













void TcpclV4BundleSource::Virtual_OnTcpclShutdownComplete_CalledFromIoServiceThread() {
    if (m_base_reconnectionDelaySecondsIfNotZero) {
        m_reconnectAfterShutdownTimer.expires_from_now(boost::posix_time::seconds(m_base_reconnectionDelaySecondsIfNotZero));
        m_reconnectAfterShutdownTimer.async_wait(boost::bind(&TcpclV4BundleSource::OnNeedToReconnectAfterShutdown_TimerExpired, this, boost::asio::placeholders::error));
    }
}

void TcpclV4BundleSource::Virtual_OnSuccessfulWholeBundleAcknowledged() {

}

//for when tcpclAllowOpportunisticReceiveBundles is set to true (not designed for extremely high throughput)
void TcpclV4BundleSource::Virtual_WholeBundleReady(padded_vector_uint8_t & wholeBundleVec) {
    if (m_outductOpportunisticProcessReceivedBundleCallback) {
        m_outductOpportunisticProcessReceivedBundleCallback(wholeBundleVec);
    }
    else {
        LOG_INFO(subprocess) << "TcpclV4BundleSource should never enter DataSegmentCallback if tcpclAllowOpportunisticReceiveBundles is set to false";
    }
}



void TcpclV4BundleSource::OnNeedToReconnectAfterShutdown_TimerExpired(const boost::system::error_code& e) {
    if (e != boost::asio::error::operation_aborted) {
        // Timer was not cancelled, take necessary action.
        if (m_base_outductTelemetry.m_numTcpReconnectAttempts == 0) {
            LOG_INFO(subprocess) << "Trying to reconnect...";
        }
        ++m_base_outductTelemetry.m_numTcpReconnectAttempts;
        m_base_shutdownCalled = false;
#ifdef OPENSSL_SUPPORT_ENABLED
        m_base_tcpAsyncSenderSslPtr.reset();
        m_base_sslStreamSharedPtr = std::make_shared<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> >(m_base_ioServiceRef, m_shareableSslContextRef);
        boost::asio::async_connect(
            m_base_sslStreamSharedPtr->next_layer(),
#else
        m_base_tcpAsyncSenderPtr.reset();
        m_base_tcpSocketPtr = std::make_shared<boost::asio::ip::tcp::socket>(m_base_ioServiceRef);
        boost::asio::async_connect(
            *m_base_tcpSocketPtr,
#endif
            m_resolverResults,
            boost::bind(
                &TcpclV4BundleSource::OnConnect,
                this,
                boost::asio::placeholders::error));
    }
}

bool TcpclV4BundleSource::ReadyToForward() const {
    return m_base_readyToForward;
}
