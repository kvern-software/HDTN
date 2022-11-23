/**
 * @file ingress.h
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 * @author  Gilbert Clark
 *
 * @copyright Copyright � 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 *
 * @section DESCRIPTION
 *
 * The ingress module of HDTN is responsible for receiving bundles, decoding them, and
 * forwarding them to either the egress or storage modules.
 */

#ifndef _HDTN_INGRESS_H
#define _HDTN_INGRESS_H

#include <stdint.h>

#include "message.hpp"
//#include "util/tsc.h"
#include "zmq.hpp"

#include "CircularIndexBufferSingleProducerSingleConsumerConfigurable.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "HdtnConfig.h"
#include "InductManager.h"
#include <list>
#include <unordered_set>
#include <queue>
#include <boost/atomic.hpp>
#include "TcpclInduct.h"
#include "TcpclV4Induct.h"
#include "Telemetry.h"
#include "ingress_async_lib_export.h"


namespace hdtn {


class Ingress {
public:
    INGRESS_ASYNC_LIB_EXPORT Ingress();  // initialize message buffers
    INGRESS_ASYNC_LIB_EXPORT ~Ingress();
    INGRESS_ASYNC_LIB_EXPORT void Stop();
    INGRESS_ASYNC_LIB_EXPORT void SchedulerEventHandler();
    INGRESS_ASYNC_LIB_EXPORT bool Init(const HdtnConfig & hdtnConfig, zmq::context_t * hdtnOneProcessZmqInprocContextPtr = NULL);
private:
    INGRESS_ASYNC_LIB_NO_EXPORT bool ProcessPaddedData(uint8_t * bundleDataBegin, std::size_t bundleCurrentSize,
        std::unique_ptr<zmq::message_t> & zmqPaddedMessageUnderlyingDataUniquePtr, padded_vector_uint8_t & paddedVecMessageUnderlyingData, const bool usingZmqData, const bool needsProcessing);
    INGRESS_ASYNC_LIB_NO_EXPORT void ReadZmqAcksThreadFunc();
    INGRESS_ASYNC_LIB_NO_EXPORT void ReadTcpclOpportunisticBundlesFromEgressThreadFunc();
    INGRESS_ASYNC_LIB_NO_EXPORT void WholeBundleReadyCallback(padded_vector_uint8_t & wholeBundleVec);
    INGRESS_ASYNC_LIB_NO_EXPORT void OnNewOpportunisticLinkCallback(const uint64_t remoteNodeId, Induct * thisInductPtr);
    INGRESS_ASYNC_LIB_NO_EXPORT void OnDeletedOpportunisticLinkCallback(const uint64_t remoteNodeId);
    INGRESS_ASYNC_LIB_NO_EXPORT void SendOpportunisticLinkMessages(const uint64_t remoteNodeId, bool isAvailable);
public:
    uint64_t m_bundleCountStorage;
    boost::atomic_uint64_t m_bundleCountEgress;
    uint64_t m_bundleCount;
    boost::atomic_uint64_t m_bundleData;
    double m_elapsed;

private:
    struct EgressToIngressAckingSet {
        EgressToIngressAckingSet();
        std::size_t GetSetSize() const noexcept;
        void PushMove_ThreadSafe(const uint64_t ingressToEgressCustody);
        bool CompareAndPop_ThreadSafe(const uint64_t ingressToEgressCustody);
        void WaitUntilNotifiedOr250MsTimeout(const uint64_t waitWhileSizeGtThisValue);
        void NotifyAll();
    private:
        // Internal implementation class
        struct Impl;
        // Pointer to the internal implementation
        std::shared_ptr<Impl> m_pimpl; //shared to make it copyable (includes noncopyable mutex)
    public:
        uint64_t maxBundlesInPipeline;
        uint64_t maxBundleSizeBytesInPipeline;
        uint64_t nextHopNodeId;
        bool linkIsUp;
    };

    std::unique_ptr<zmq::context_t> m_zmqCtxPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPushSock_boundIngressToConnectingEgressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPullSock_connectingEgressToBoundIngressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPullSock_connectingEgressBundlesOnlyToBoundIngressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqPushSock_boundIngressToConnectingStoragePtr;
    std::unique_ptr<zmq::socket_t> m_zmqPullSock_connectingStorageToBoundIngressPtr;
    std::unique_ptr<zmq::socket_t> m_zmqSubSock_boundSchedulerToConnectingIngressPtr;

    std::unique_ptr<zmq::socket_t> m_zmqRepSock_connectingGuiToFromBoundIngressPtr;

    //std::shared_ptr<zmq::context_t> m_zmqTelemCtx;
    //std::shared_ptr<zmq::socket_t> m_zmqTelemSock;

    InductManager m_inductManager;
    HdtnConfig m_hdtnConfig;
    cbhe_eid_t M_HDTN_EID_CUSTODY;
    cbhe_eid_t M_HDTN_EID_ECHO;
    boost::posix_time::time_duration M_MAX_INGRESS_BUNDLE_WAIT_ON_EGRESS_TIME_DURATION;
    
    std::unique_ptr<boost::thread> m_threadZmqAckReaderPtr;
    std::unique_ptr<boost::thread> m_threadTcpclOpportunisticBundlesFromEgressReaderPtr;
    std::queue<uint64_t> m_storageAckQueue;
    boost::mutex m_storageAckQueueMutex;
    boost::condition_variable m_conditionVariableStorageAckReceived;
    std::vector<EgressToIngressAckingSet> m_vectorEgressToIngressAckingSet; //final dest node id to set
    std::map<uint64_t, uint64_t> m_mapNextHopNodeIdToOutductArrayIndex;
    std::map<uint64_t, uint64_t> m_mapFinalDestNodeIdToOutductArrayIndex;
    std::map<cbhe_eid_t, uint64_t> m_mapFinalDestEidToOutductArrayIndex;

    struct SharedMutexImpl;
    std::unique_ptr<SharedMutexImpl> m_ptrSharedMutexImpl;



    boost::mutex m_ingressToEgressZmqSocketMutex;
    std::size_t m_eventsTooManyInStorageQueue;
    std::size_t m_eventsTooManyInEgressQueue;
    volatile bool m_running;
    volatile bool m_egressFullyInitialized;
    boost::atomic_uint64_t m_ingressToEgressNextUniqueIdAtomic;
    uint64_t m_ingressToStorageNextUniqueId;

    std::map<uint64_t, Induct*> m_availableDestOpportunisticNodeIdToTcpclInductMap;
    boost::mutex m_availableDestOpportunisticNodeIdToTcpclInductMapMutex;
};


}  // namespace hdtn

#endif  //_HDTN_INGRESS_H
