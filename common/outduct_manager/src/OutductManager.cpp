/**
 * @file OutductManager.cpp
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

#include "OutductManager.h"
#include "Logger.h"
#include <boost/make_unique.hpp>
#include <memory>
#include "TcpclOutduct.h"
#include "TcpclV4Outduct.h"
#include "StcpOutduct.h"
#include "UdpOutduct.h"
#include "LtpOverUdpOutduct.h"
#include "LtpOverIpcOutduct.h"
#include "Uri.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

OutductManager::OutductManager() : m_numEventsTooManyUnackedBundles(0) {}

OutductManager::~OutductManager() {
    LOG_INFO(subprocess) << "m_numEventsTooManyUnackedBundles " << m_numEventsTooManyUnackedBundles;
}

bool OutductManager::LoadOutductsFromConfig(const OutductsConfig & outductsConfig, const uint64_t myNodeId, const uint64_t maxUdpRxPacketSizeBytesForAllLtp,
    const uint64_t maxOpportunisticRxBundleSizeBytes,
    const OutductOpportunisticProcessReceivedBundleCallback_t & outductOpportunisticProcessReceivedBundleCallback,
    const OnFailedBundleVecSendCallback_t& outductOnFailedBundleVecSendCallback,
    const OnFailedBundleZmqSendCallback_t& outductOnFailedBundleZmqSendCallback,
    const OnSuccessfulBundleSendCallback_t& onSuccessfulBundleSendCallback,
    const OnOutductLinkStatusChangedCallback_t& onOutductLinkStatusChangedCallback)
{
    LtpUdpEngineManager::SetMaxUdpRxPacketSizeBytesForAllLtp(maxUdpRxPacketSizeBytesForAllLtp); //MUST BE CALLED BEFORE ANY USAGE OF LTP
    m_finalDestEidToOutductMap.clear();
    m_finalDestNodeIdToOutductMap.clear();
    m_nextHopNodeIdToOutductMap.clear();
    m_outductsVec.clear();
    uint64_t nextOutductUuidIndex = 0;
    const outduct_element_config_vector_t & configsVec = outductsConfig.m_outductElementConfigVector;
    m_outductsVec.reserve(configsVec.size());
    for (outduct_element_config_vector_t::const_iterator it = configsVec.cbegin(); it != configsVec.cend(); ++it) {
        const outduct_element_config_t & thisOutductConfig = *it;
        std::shared_ptr<Outduct> outductSharedPtr;
        const uint64_t uuidIndex = nextOutductUuidIndex;
        //an error shall be thrown if (maxBundleSizeBytes * 2) > maxSumOfBundleBytesInPipeline
        //to prevent a large bundle from being unable to be sent, since hdtn cannot fragment bundles as of now
        //note: function parameter maxOpportunisticRxBundleSizeBytes is set from m_hdtnConfig.m_maxBundleSizeBytes, so use that
        const uint64_t maxBundleSizeBytes = maxOpportunisticRxBundleSizeBytes;
        if ((maxBundleSizeBytes * 2) > thisOutductConfig.maxSumOfBundleBytesInPipeline) {
            LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: maxBundleSizeBytes("
                << maxBundleSizeBytes
                << ") x 2 > maxSumOfBundleBytesInPipeline("
                << thisOutductConfig.maxSumOfBundleBytesInPipeline
                << ") for outduct index " << uuidIndex;
            return false;
        }
        if (thisOutductConfig.convergenceLayer == "tcpcl_v3") {
            if (thisOutductConfig.tcpclAllowOpportunisticReceiveBundles) {
                outductSharedPtr = std::make_shared<TcpclOutduct>(thisOutductConfig, myNodeId, uuidIndex, outductOpportunisticProcessReceivedBundleCallback);
            }
            else {
                outductSharedPtr = std::make_shared<TcpclOutduct>(thisOutductConfig, myNodeId, uuidIndex);
            }
        }
        else if (thisOutductConfig.convergenceLayer == "tcpcl_v4") {
#ifndef OPENSSL_SUPPORT_ENABLED
            if (thisOutductConfig.tlsIsRequired) {
                LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: TLS is required for this tcpcl v4 outduct but HDTN is not compiled with OpenSSL support.";
                return false;
            }
#endif
            if (thisOutductConfig.tcpclAllowOpportunisticReceiveBundles) {
                outductSharedPtr = std::make_shared<TcpclV4Outduct>(thisOutductConfig, myNodeId, uuidIndex, maxOpportunisticRxBundleSizeBytes, outductOpportunisticProcessReceivedBundleCallback);
            }
            else {
                outductSharedPtr = std::make_shared<TcpclV4Outduct>(thisOutductConfig, myNodeId, uuidIndex, maxOpportunisticRxBundleSizeBytes);
            }
        }
        else if (thisOutductConfig.convergenceLayer == "stcp") {
            outductSharedPtr = std::make_shared<StcpOutduct>(thisOutductConfig, uuidIndex);
        }
        else if (thisOutductConfig.convergenceLayer == "udp") {
            outductSharedPtr = std::make_shared<UdpOutduct>(thisOutductConfig, uuidIndex);
        }
        else if (thisOutductConfig.convergenceLayer == "ltp_over_udp") {
            outductSharedPtr = std::make_shared<LtpOverUdpOutduct>(thisOutductConfig, uuidIndex);
        }
        else if (thisOutductConfig.convergenceLayer == "ltp_over_ipc") {
            outductSharedPtr = std::make_shared<LtpOverIpcOutduct>(thisOutductConfig, uuidIndex);
        }

        if (outductSharedPtr) {
            if (!outductSharedPtr->Init()) {
                LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: convergence layer " << thisOutductConfig.convergenceLayer << " failed to initialize.";
                return false;
            }
            ++nextOutductUuidIndex;
            for (std::set<std::string>::const_iterator itDestUri = thisOutductConfig.finalDestinationEidUris.cbegin(); itDestUri != thisOutductConfig.finalDestinationEidUris.cend(); ++itDestUri) {
                const std::string & finalDestinationEidUri = *itDestUri;
                const bool isFirstLoop = (itDestUri == thisOutductConfig.finalDestinationEidUris.cbegin());
                cbhe_eid_t destEid;
                bool serviceNumberIsWildCard;
                if (!Uri::ParseIpnUriString(finalDestinationEidUri, destEid.nodeId, destEid.serviceId, &serviceNumberIsWildCard)) {
                    LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: finalDestinationEidUri " << finalDestinationEidUri
                        << " is invalid.";
                    return false;
                }
                if (serviceNumberIsWildCard) {
                    if (m_finalDestNodeIdToOutductMap.emplace(destEid.nodeId, outductSharedPtr).second == false) { //not inserted, already exists
                        LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: finalDestinationEidUri " << finalDestinationEidUri
                            << " is already in use by another outduct.";
                        return false;
                    }
                    //make sure there is nothing in the fully qualified map
                    const cbhe_eid_t eidStart = cbhe_eid_t(destEid.nodeId, 0);
                    //lower bound points to equivalent or next greater
                    std::map<cbhe_eid_t, std::shared_ptr<Outduct> >::iterator eidIt = m_finalDestEidToOutductMap.lower_bound(eidStart);
                    if ((eidIt != m_finalDestEidToOutductMap.end()) && (eidIt->first.nodeId == destEid.nodeId)) {
                        LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: finalDestinationEidUri " << finalDestinationEidUri 
                            << " is already in use by an outduct using the fully qualified eid " << eidIt->first;
                        return false;
                    }
                }
                else { //fully qualified eid
                    if (isFirstLoop) {
                        //this is for an outduct that may only want to forward certain service numbers and reject bundles with unknown service numbers
                        //make sure there is nothing in the fully qualified map
                        const cbhe_eid_t eidStart = cbhe_eid_t(destEid.nodeId, 0);
                        //lower bound points to equivalent or next greater
                        std::map<cbhe_eid_t, std::shared_ptr<Outduct> >::iterator eidIt = m_finalDestEidToOutductMap.lower_bound(eidStart);
                        if ((eidIt != m_finalDestEidToOutductMap.end()) && (eidIt->first.nodeId == destEid.nodeId)) {
                            LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: finalDestinationEidUri " << finalDestinationEidUri
                                << " shares a node number that is already in use by an outduct using the fully qualified eid " << eidIt->first 
                                << ". Please note that HDTN does not support selecting an outduct by service number.";
                            return false;
                        }

                    }
                    if (m_finalDestEidToOutductMap.emplace(destEid, outductSharedPtr).second == false) { //not inserted, already exists
                        LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: finalDestinationEidUri " << finalDestinationEidUri
                            << " is already in use by another outduct.";
                        return false;
                    }
                    if (m_finalDestNodeIdToOutductMap.count(destEid.nodeId)) {
                        LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: finalDestinationEidUri " << finalDestinationEidUri
                            << " is already in use by an outduct using the wildcard eid " 
                            << Uri::GetIpnUriStringAnyServiceNumber(destEid.nodeId);
                        return false;
                    }
                }
            }
            if (m_nextHopNodeIdToOutductMap.emplace(thisOutductConfig.nextHopNodeId, outductSharedPtr).second == false) { //not inserted, already exists
                LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: nextHopNodeId " << thisOutductConfig.nextHopNodeId
                    << " is already in use by another outduct.";
                return false;
            }

            outductSharedPtr->SetOnFailedBundleVecSendCallback(outductOnFailedBundleVecSendCallback);
            outductSharedPtr->SetOnFailedBundleZmqSendCallback(outductOnFailedBundleZmqSendCallback);
            outductSharedPtr->SetOnSuccessfulBundleSendCallback(onSuccessfulBundleSendCallback);
            outductSharedPtr->SetOnOutductLinkStatusChangedCallback(onOutductLinkStatusChangedCallback);
            outductSharedPtr->SetUserAssignedUuid(uuidIndex);

            //for any connection-oriented convergence layer, initially send link down event,
            // and when connection completes, the convergence layer will send a link up event
            if ((thisOutductConfig.convergenceLayer == "tcpcl_v3") || (thisOutductConfig.convergenceLayer == "tcpcl_v4") || (thisOutductConfig.convergenceLayer == "stcp")) {
                if (onOutductLinkStatusChangedCallback) { //send initial link down event
                    LOG_DEBUG(subprocess) << "Outduct index(" << uuidIndex
                        << ") with connection-oriented convergence layer " << thisOutductConfig.convergenceLayer
                        << " setting link down before calling Connect()";
                    onOutductLinkStatusChangedCallback(true, uuidIndex);
                }
            }
            /* the following has issues with bpgen
            else if ((thisOutductConfig.convergenceLayer == "ltp_over_udp") && (thisOutductConfig.ltpSenderPingSecondsOrZeroToDisable != 0)) {
                if (onOutductLinkStatusChangedCallback) { //send initial link down event
                    LOG_DEBUG(subprocess) << "Outduct index(" << uuidIndex
                        << ") with ltp convergence layer (WITH ltp ping enabled) setting link down before calling Connect()";
                    onOutductLinkStatusChangedCallback(true, uuidIndex);
                }
            }
            */

            outductSharedPtr->Connect();
            m_outductsVec.push_back(std::move(outductSharedPtr)); //uuid will be the array index
        }
        else {
            LOG_ERROR(subprocess) << "OutductManager::LoadOutductsFromConfig: convergence layer " << thisOutductConfig.convergenceLayer << " is invalid.";
            return false;
        }
    }
    return true;
}

bool OutductManager::AllReadyToForward() const {
    for (std::vector<std::shared_ptr<Outduct> >::const_iterator it = m_outductsVec.cbegin(); it != m_outductsVec.cend(); ++it) {
        const std::shared_ptr<Outduct> & outductPtr = *it;
        if (!outductPtr->ReadyToForward()) {
            return false;
        }
    }
    return true;
}

void OutductManager::StopAllOutducts() {
    for (std::vector<std::shared_ptr<Outduct> >::const_iterator it = m_outductsVec.cbegin(); it != m_outductsVec.cend(); ++it) {
        const std::shared_ptr<Outduct> & outductPtr = *it;
        outductPtr->Stop();
    }
}

bool OutductManager::Forward(const cbhe_eid_t & finalDestEid, const uint8_t* bundleData, const std::size_t size, std::vector<uint8_t>&& userData) {
    if (Outduct * const outductPtr = GetOutductByFinalDestinationEid_ThreadSafe(finalDestEid)) {
        if (outductPtr->GetTotalDataSegmentsUnacked() > outductPtr->GetOutductMaxNumberOfBundlesInPipeline()) {
            LOG_ERROR(subprocess) << "bundle pipeline limit exceeded";
            return false;
        }
        else {
            return outductPtr->Forward(bundleData, size, std::move(userData));
        }
    }
    else {
        LOG_ERROR(subprocess) << "OutductManager::Forward: finalDestEid (" << finalDestEid.nodeId << "," << finalDestEid.serviceId << ") not found.";
        return false;
    }
}
bool OutductManager::Forward(const cbhe_eid_t & finalDestEid, zmq::message_t & movableDataZmq, std::vector<uint8_t>&& userData) {
    if (Outduct * const outductPtr = GetOutductByFinalDestinationEid_ThreadSafe(finalDestEid)) {
        if (outductPtr->GetTotalDataSegmentsUnacked() > outductPtr->GetOutductMaxNumberOfBundlesInPipeline()) {
            LOG_ERROR(subprocess) << "bundle pipeline limit exceeded";
            return false;
        }
        else {
            return outductPtr->Forward(movableDataZmq, std::move(userData));
        }
    }
    else {
        LOG_ERROR(subprocess) << "OutductManager::Forward: finalDestEid (" << finalDestEid.nodeId << "," << finalDestEid.serviceId << ") not found.";
        return false;
    }
}
bool OutductManager::Forward(const cbhe_eid_t & finalDestEid, std::vector<uint8_t> & movableDataVec, std::vector<uint8_t>&& userData) {
    if (Outduct * const outductPtr = GetOutductByFinalDestinationEid_ThreadSafe(finalDestEid)) {
        if (outductPtr->GetTotalDataSegmentsUnacked() > outductPtr->GetOutductMaxNumberOfBundlesInPipeline()) {
            LOG_ERROR(subprocess) << "bundle pipeline limit exceeded";
            return false;
        }
        else {
            return outductPtr->Forward(movableDataVec, std::move(userData));
        }
    }
    else {
        LOG_ERROR(subprocess) << "OutductManager::Forward: finalDestEid (" << finalDestEid.nodeId << "," << finalDestEid.serviceId << ") not found.";
        return false;
    }
}

bool OutductManager::Reroute_ThreadSafe(const uint64_t finalDestNodeId, const uint64_t newNextHopNodeId) {

    boost::mutex::scoped_lock lock(m_finalDestNodeIdToOutductMapMutex);
    boost::mutex::scoped_lock lock2(m_finalDestEidToOutductMapMutex);

    std::map<uint64_t, std::shared_ptr<Outduct> >::iterator itNextHopNodeId = m_nextHopNodeIdToOutductMap.find(newNextHopNodeId);
    if (itNextHopNodeId == m_nextHopNodeIdToOutductMap.end()) {
        LOG_ERROR(subprocess) << "OutductManager::Reroute_ThreadSafe: newNextHopNodeId " << newNextHopNodeId << " not found in HDTN's outducts";
        return false;
    }
    std::shared_ptr<Outduct> & newOutductPtrRef = itNextHopNodeId->second;

    //first check if wild card
    bool success = false;
    std::map<uint64_t, std::shared_ptr<Outduct> >::iterator itNodeMap = m_finalDestNodeIdToOutductMap.find(finalDestNodeId);
    if (itNodeMap != m_finalDestNodeIdToOutductMap.end()) { //this map contains eids to move to another 
        itNodeMap->second = newOutductPtrRef;
        success = true;
    }
    else { //fully qualified eids
        const cbhe_eid_t eidStart = cbhe_eid_t(finalDestNodeId, 0);
        typedef std::map<cbhe_eid_t, std::shared_ptr<Outduct> >::iterator it_t; //for deletion later
        typedef std::pair<it_t, std::shared_ptr<Outduct> > it_outduct_pair_t;
        std::queue<it_outduct_pair_t> outductsToMove;
        //lower bound points to equivalent or next greater
        for (std::map<cbhe_eid_t, std::shared_ptr<Outduct> >::iterator itEidMap = m_finalDestEidToOutductMap.lower_bound(eidStart);
            (itEidMap != m_finalDestEidToOutductMap.end()) && (itEidMap->first.nodeId == finalDestNodeId);
            ++itEidMap)
        {
            itEidMap->second = newOutductPtrRef;
            success = true;
        }
    }
    return success;
}

void OutductManager::GetAllOutductCapabilitiesTelemetry_ThreadSafe(AllOutductCapabilitiesTelemetry_t& allOutductCapabilitiesTelemetry) {
    std::vector<OutductCapabilityTelemetry_t> octVec(m_outductsVec.size());
    allOutductCapabilitiesTelemetry.outductCapabilityTelemetryList.clear();
    {
        boost::mutex::scoped_lock lock(m_finalDestNodeIdToOutductMapMutex);
        boost::mutex::scoped_lock lock2(m_finalDestEidToOutductMapMutex);
        for (std::map<uint64_t, std::shared_ptr<Outduct> >::const_iterator it = m_finalDestNodeIdToOutductMap.cbegin(); it != m_finalDestNodeIdToOutductMap.cend(); ++it) {
            const uint64_t nodeId = it->first;
            const uint64_t outductIndex = it->second->GetOutductUuid();
            OutductCapabilityTelemetry_t& oct = octVec[outductIndex];
            oct.finalDestinationNodeIdList.emplace_back(nodeId);
        }
        for (std::map<cbhe_eid_t, std::shared_ptr<Outduct> >::const_iterator it = m_finalDestEidToOutductMap.cbegin(); it != m_finalDestEidToOutductMap.cend(); ++it) {
            const cbhe_eid_t& eid = it->first;
            const uint64_t outductIndex = it->second->GetOutductUuid();
            OutductCapabilityTelemetry_t& oct = octVec[outductIndex];
            oct.finalDestinationEidList.emplace_back(eid);
        }
    }
    //convert vector to list
    for (std::size_t i = 0; i < octVec.size(); ++i) {
        OutductCapabilityTelemetry_t& oct = octVec[i];
        std::shared_ptr<Outduct> & outductPtr = m_outductsVec[i];
        oct.maxBundlesInPipeline = outductPtr->GetOutductMaxNumberOfBundlesInPipeline();
        oct.maxBundleSizeBytesInPipeline = outductPtr->GetOutductMaxSumOfBundleBytesInPipeline();
        oct.outductArrayIndex = i;
        oct.nextHopNodeId = outductPtr->GetOutductNextHopNodeId();
        allOutductCapabilitiesTelemetry.outductCapabilityTelemetryList.emplace_back(std::move(oct));
    }
}

Outduct * OutductManager::GetOutductByFinalDestinationEid_ThreadSafe(const cbhe_eid_t & finalDestEid) {
    Outduct* retVal = NULL;
    //first try to find by node id (ipn:nodeId.*)
    m_finalDestNodeIdToOutductMapMutex.lock();
    std::map<uint64_t, std::shared_ptr<Outduct> >::iterator itNodeIdOutduct = m_finalDestNodeIdToOutductMap.find(finalDestEid.nodeId);
    if (itNodeIdOutduct != m_finalDestNodeIdToOutductMap.end()) {
        retVal = itNodeIdOutduct->second.get();
    }
    m_finalDestNodeIdToOutductMapMutex.unlock();
    if (retVal == NULL) { //try to find by fully qualified eid
        m_finalDestEidToOutductMapMutex.lock();
        std::map<cbhe_eid_t, std::shared_ptr<Outduct> >::iterator itEidOutduct = m_finalDestEidToOutductMap.find(finalDestEid);
        if (itEidOutduct != m_finalDestEidToOutductMap.end()) {
            retVal = itEidOutduct->second.get();
        }
        m_finalDestEidToOutductMapMutex.unlock();
    }
    return retVal;
}

Outduct * OutductManager::GetOutductByNextHopNodeId(const uint64_t nextHopNodeId) {
    try {
        if (std::shared_ptr<Outduct> & outductPtr = m_nextHopNodeIdToOutductMap.at(nextHopNodeId)) {
            return outductPtr.get();
        }
    }
    catch (const std::out_of_range &) {}
    return NULL;
}

Outduct * OutductManager::GetOutductByOutductUuid(const uint64_t uuid) {
    try {
        if (std::shared_ptr<Outduct> & outductPtr = m_outductsVec.at(uuid)) {
            return outductPtr.get();
        }
    }
    catch (const std::out_of_range &) {}
    
    return NULL;
}

std::shared_ptr<Outduct> OutductManager::GetOutductSharedPtrByOutductUuid(const uint64_t uuid) {
    try {
        if (std::shared_ptr<Outduct> & outductPtr = m_outductsVec.at(uuid)) {
            return outductPtr;
        }
    }
    catch (const std::out_of_range &) {}

    return NULL;
}

void OutductManager::PopulateAllOutductTelemetry(AllOutductTelemetry_t& allOutductTelem) {
    allOutductTelem.m_listAllOutducts.clear();
    allOutductTelem.m_totalBundlesSuccessfullySent = 0;
    allOutductTelem.m_totalBundleBytesSuccessfullySent = 0;
    for (std::vector<std::shared_ptr<Outduct> >::const_iterator it = m_outductsVec.cbegin(); it != m_outductsVec.cend(); ++it) {
        allOutductTelem.m_listAllOutducts.emplace_back();
        std::unique_ptr<OutductTelemetry_t>& otPtr = allOutductTelem.m_listAllOutducts.back();
        (*it)->PopulateOutductTelemetry(otPtr);
        if (!otPtr) {
            allOutductTelem.m_listAllOutducts.pop_back();
        }
        else {
            allOutductTelem.m_totalBundlesSuccessfullySent += otPtr->m_totalBundlesAcked;
            allOutductTelem.m_totalBundleBytesSuccessfullySent += otPtr->m_totalBundleBytesAcked;
        }
    }
    allOutductTelem.m_timestampMilliseconds = TimestampUtil::GetMillisecondsSinceEpochRfc5050();
}
