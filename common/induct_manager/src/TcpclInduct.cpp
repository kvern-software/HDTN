/**
 * @file TcpclInduct.cpp
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright � 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 */

#include "TcpclInduct.h"
#include "Logger.h"
#include <boost/make_unique.hpp>
#include <boost/lexical_cast.hpp>
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

//TCPCL INDUCT
TcpclInduct::TcpclInduct(const InductProcessBundleCallback_t & inductProcessBundleCallback, const induct_element_config_t & inductConfig,
    const uint64_t myNodeId, const uint64_t maxBundleSizeBytes, const OnNewOpportunisticLinkCallback_t & onNewOpportunisticLinkCallback,
    const OnDeletedOpportunisticLinkCallback_t & onDeletedOpportunisticLinkCallback) :
    Induct(inductProcessBundleCallback, inductConfig),
    m_tcpAcceptor(m_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), inductConfig.boundPort)),
    m_workPtr(boost::make_unique<boost::asio::io_service::work>(m_ioService)),
    M_MY_NODE_ID(myNodeId),
    m_allowRemoveInactiveTcpConnections(true),
    M_MAX_BUNDLE_SIZE_BYTES(maxBundleSizeBytes)    
{
    m_onNewOpportunisticLinkCallback = onNewOpportunisticLinkCallback;
    m_onDeletedOpportunisticLinkCallback = onDeletedOpportunisticLinkCallback;
    StartTcpAccept();
    m_ioServiceThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_ioService));
    ThreadNamer::SetIoServiceThreadName(m_ioService, "ioServiceTcpclInduct");
}
TcpclInduct::~TcpclInduct() {
    if (m_tcpAcceptor.is_open()) {
        try {
            m_tcpAcceptor.close();
        }
        catch (const boost::system::system_error & e) {
            LOG_ERROR(subprocess) << "Error closing TCP Acceptor in TcpclInduct::~TcpclInduct:  " << e.what();
        }
    }
    boost::asio::post(m_ioService, boost::bind(&TcpclInduct::DisableRemoveInactiveTcpConnections, this));
    while (m_allowRemoveInactiveTcpConnections) {
        try {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }
        catch (const boost::thread_resource_error&) {}
        catch (const boost::thread_interrupted&) {}
        catch (const boost::condition_error&) {}
        catch (const boost::lock_error&) {}
    }
    m_listTcpclBundleSinks.clear(); //tcp bundle sink destructor is thread safe
    m_workPtr.reset();

    if (m_ioServiceThreadPtr) {
        try {
            m_ioServiceThreadPtr->join();
            m_ioServiceThreadPtr.reset(); //delete it
        }
        catch (const boost::thread_resource_error&) {
            LOG_ERROR(subprocess) << "error stopping TcpclInduct io_service";
        }
    }
}

void TcpclInduct::StartTcpAccept() {
    LOG_INFO(subprocess) << "waiting for tcpcl tcp connections";
    std::shared_ptr<boost::asio::ip::tcp::socket> newTcpSocketPtr = std::make_shared<boost::asio::ip::tcp::socket>(m_ioService); //get_io_service() is deprecated: Use get_executor()

    m_tcpAcceptor.async_accept(*newTcpSocketPtr,
        boost::bind(&TcpclInduct::HandleTcpAccept, this, newTcpSocketPtr, boost::asio::placeholders::error));
}

void TcpclInduct::HandleTcpAccept(std::shared_ptr<boost::asio::ip::tcp::socket> & newTcpSocketPtr, const boost::system::error_code& error) {
    if (!error) {
        LOG_INFO(subprocess) << "tcpcl tcp connection: " << newTcpSocketPtr->remote_endpoint().address() << ":" << newTcpSocketPtr->remote_endpoint().port();
        {
            boost::mutex::scoped_lock lock(m_listTcpclBundleSinksMutex);
            m_listTcpclBundleSinks.emplace_back(
                m_inductConfig.keepAliveIntervalSeconds,
                newTcpSocketPtr,
                m_ioService,
                m_inductProcessBundleCallback,
                m_inductConfig.numRxCircularBufferElements,
                m_inductConfig.numRxCircularBufferBytesPerElement,
                M_MY_NODE_ID,
                M_MAX_BUNDLE_SIZE_BYTES,
                boost::bind(&TcpclInduct::ConnectionReadyToBeDeletedNotificationReceived, this),
                boost::bind(&TcpclInduct::OnContactHeaderCallback_FromIoServiceThread, this, boost::placeholders::_1),
                10, //const unsigned int maxUnacked, (todo)
                m_inductConfig.tcpclV3MyMaxTxSegmentSizeBytes); //const uint64_t maxFragmentSize = 100000000); (todo)
        }
        StartTcpAccept(); //only accept if there was no error
    }
    else if (error != boost::asio::error::operation_aborted) {
        LOG_ERROR(subprocess) << "tcp accept error: " << error.message();
    }


}

void TcpclInduct::RemoveInactiveTcpConnections() {
    const OnDeletedOpportunisticLinkCallback_t & callbackRef = m_onDeletedOpportunisticLinkCallback;
    //std::map<uint64_t, OpportunisticBundleQueue> & mapNodeIdToOpportunisticBundleQueueRef = m_mapNodeIdToOpportunisticBundleQueue;
    //boost::mutex & mapNodeIdToOpportunisticBundleQueueMutexRef = m_mapNodeIdToOpportunisticBundleQueueMutex;
    if (m_allowRemoveInactiveTcpConnections) {
        boost::mutex::scoped_lock lock(m_listTcpclBundleSinksMutex);
        m_listTcpclBundleSinks.remove_if([&callbackRef, this/*, &mapNodeIdToOpportunisticBundleQueueMutexRef, &mapNodeIdToOpportunisticBundleQueueRef*/](TcpclBundleSink & sink) {
            if (sink.ReadyToBeDeleted()) {
                if (callbackRef) {
                    callbackRef(sink.GetRemoteNodeId(), this, &sink);
                }
                //mapNodeIdToOpportunisticBundleQueueMutexRef.lock();
                //mapNodeIdToOpportunisticBundleQueueRef.erase(sink.GetRemoteNodeId());
                //mapNodeIdToOpportunisticBundleQueueMutexRef.unlock();
                return true;
            }
            else {
                return false;
            }
        });
    }
}

void TcpclInduct::DisableRemoveInactiveTcpConnections() {
    m_allowRemoveInactiveTcpConnections = false;
}

void TcpclInduct::ConnectionReadyToBeDeletedNotificationReceived() {
    boost::asio::post(m_ioService, boost::bind(&TcpclInduct::RemoveInactiveTcpConnections, this));
}





void TcpclInduct::OnContactHeaderCallback_FromIoServiceThread(TcpclBundleSink * thisTcpclBundleSinkPtr) {
    m_mapNodeIdToOpportunisticBundleQueueMutex.lock();
    m_mapNodeIdToOpportunisticBundleQueue.erase(thisTcpclBundleSinkPtr->GetRemoteNodeId());
    OpportunisticBundleQueue & opportunisticBundleQueue = m_mapNodeIdToOpportunisticBundleQueue[thisTcpclBundleSinkPtr->GetRemoteNodeId()];
    //opportunisticBundleQueue.m_bidirectionalLinkPtr = thisTcpclBundleSinkPtr;
    opportunisticBundleQueue.m_maxTxBundlesInPipeline = thisTcpclBundleSinkPtr->Virtual_GetMaxTxBundlesInPipeline();
    opportunisticBundleQueue.m_remoteNodeId = thisTcpclBundleSinkPtr->GetRemoteNodeId();
    m_mapNodeIdToOpportunisticBundleQueueMutex.unlock();
    thisTcpclBundleSinkPtr->SetTryGetOpportunisticDataFunction(boost::bind(&TcpclInduct::BundleSinkTryGetData_FromIoServiceThread, this, boost::ref(opportunisticBundleQueue), boost::placeholders::_1));
    thisTcpclBundleSinkPtr->SetNotifyOpportunisticDataAckedCallback(boost::bind(&TcpclInduct::BundleSinkNotifyOpportunisticDataAcked_FromIoServiceThread, this, boost::ref(opportunisticBundleQueue)));
    if (m_onNewOpportunisticLinkCallback) {
        m_onNewOpportunisticLinkCallback(thisTcpclBundleSinkPtr->GetRemoteNodeId(), this, thisTcpclBundleSinkPtr);
    }
}

void TcpclInduct::NotifyBundleReadyToSend_FromIoServiceThread(const uint64_t remoteNodeId) {
    for (std::list<TcpclBundleSink>::iterator it = m_listTcpclBundleSinks.begin(); it != m_listTcpclBundleSinks.end(); ++it) {
        if (it->GetRemoteNodeId() == remoteNodeId) {
            it->TrySendOpportunisticBundleIfAvailable_FromIoServiceThread();
        }
    }
}

void TcpclInduct::Virtual_PostNotifyBundleReadyToSend_FromIoServiceThread(const uint64_t remoteNodeId) {
    boost::asio::post(m_ioService, boost::bind(&TcpclInduct::NotifyBundleReadyToSend_FromIoServiceThread, this, remoteNodeId));
}

void TcpclInduct::PopulateInductTelemetry(InductTelemetry_t& inductTelem) {
    inductTelem.m_convergenceLayer = "tcpcl_v3";
    inductTelem.m_listInductConnections.clear();
    {
        boost::mutex::scoped_lock lock(m_listTcpclBundleSinksMutex);
        for (std::list<TcpclBundleSink>::const_iterator it = m_listTcpclBundleSinks.cbegin(); it != m_listTcpclBundleSinks.cend(); ++it) {
            inductTelem.m_listInductConnections.emplace_back(boost::make_unique<TcpclV3InductConnectionTelemetry_t>(it->m_base_inductConnectionTelemetry));
        }
    }
    if (inductTelem.m_listInductConnections.empty()) {
        std::unique_ptr<TcpclV3InductConnectionTelemetry_t> c = boost::make_unique<TcpclV3InductConnectionTelemetry_t>();
        c->m_connectionName = "null";
        c->m_inputName = std::string("*:") + boost::lexical_cast<std::string>(m_tcpAcceptor.local_endpoint().port());
        inductTelem.m_listInductConnections.emplace_back(std::move(c));
    }
}
