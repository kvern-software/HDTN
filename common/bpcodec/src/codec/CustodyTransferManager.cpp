#include "codec/CustodyTransferManager.h"
#include <iostream>

#include "Uri.h"

static const bool INDEX_TO_IS_SUCCESS[NUM_ACS_STATUS_INDICES] = {
    true,
    false,
    false,
    false,
    false,
    false,
    false
};
static const BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT INDEX_TO_REASON_CODE[NUM_ACS_STATUS_INDICES] = {
    BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::NO_ADDITIONAL_INFORMATION,
    BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::REDUNDANT_RECEPTION,
    BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::DEPLETED_STORAGE,
    BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::DESTINATION_ENDPOINT_ID_UNINTELLIGIBLE,
    BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::NO_KNOWN_ROUTE_TO_DESTINATION_FROM_HERE,
    BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::NO_TIMELY_CONTACT_WITH_NEXT_NODE_ON_ROUTE,
    BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::BLOCK_UNINTELLIGIBLE
};

bool CustodyTransferManager::GenerateCustodySignalBundle(std::vector<uint8_t> & serializedBundle, const bpv6_primary_block & primaryFromSender, const BPV6_ACS_STATUS_REASON_INDICES statusReasonIndex) const {
    serializedBundle.resize(CBHE_BPV6_MINIMUM_SAFE_PRIMARY_HEADER_ENCODE_SIZE + CustodySignal::CBHE_MAX_SERIALIZATION_SIZE);
    uint8_t * const serializationBase = &serializedBundle[0];
    uint8_t * buffer = serializationBase;

    bpv6_primary_block primary;
    memset(&primary, 0, sizeof(bpv6_primary_block));
    primary.version = 6;
    bpv6_canonical_block block;
    memset(&block, 0, sizeof(bpv6_canonical_block));

    primary.flags = bpv6_bundle_set_priority(bpv6_bundle_get_priority(primaryFromSender.flags)) |
        bpv6_bundle_set_gflags(BPV6_BUNDLEFLAG_SINGLETON | BPV6_BUNDLEFLAG_NOFRAGMENT | BPV6_BUNDLEFLAG_ADMIN_RECORD);
    primary.src_node = m_myCustodianNodeId;
    primary.src_svc = m_myCustodianServiceId;
    primary.dst_node = primaryFromSender.custodian_node;
    primary.dst_svc = primaryFromSender.custodian_svc;
    primary.custodian_node = 0;
    primary.custodian_svc = 0;
    primary.creation = 1000;//TODO //(uint64_t)bpv6_unix_to_5050(curr_time);
    primary.lifetime = 1000;
    primary.sequence = 1;
    uint64_t retVal;
    retVal = cbhe_bpv6_primary_block_encode(&primary, (char *)buffer, 0, 0);
    if (retVal == 0) {
        return false;
    }
    buffer += retVal;

    CustodySignal sig;
    sig.m_copyOfBundleCreationTimestampTimeSeconds = primaryFromSender.creation;
    sig.m_copyOfBundleCreationTimestampSequenceNumber = primaryFromSender.sequence;
    sig.m_bundleSourceEid = Uri::GetIpnUriString(primaryFromSender.custodian_node, primaryFromSender.custodian_svc);
    //REQ D4.2.2.7 An ACS-aware bundle protocol agent shall utilize the ACS bundle timestamp
    //time as the �Time of Signal� when executing RFC 5050 section 6.3
    sig.SetTimeOfSignalGeneration(TimestampUtil::GenerateDtnTimeNow());//add custody
    const uint8_t sri = static_cast<uint8_t>(statusReasonIndex);
    sig.SetCustodyTransferStatusAndReason(INDEX_TO_IS_SUCCESS[sri], INDEX_TO_REASON_CODE[sri]);
    uint64_t sizeSerialized = sig.Serialize(buffer);
    buffer += sizeSerialized;

    serializedBundle.resize(buffer - serializationBase);
    return true;
}
bool CustodyTransferManager::GenerateAcsBundle(std::vector<uint8_t> & serializedBundle, const bpv6_primary_block & primaryFromSender, const BPV6_ACS_STATUS_REASON_INDICES statusReasonIndex) const {
    const AggregateCustodySignal & currentAcs = m_acsArray[static_cast<uint8_t>(statusReasonIndex)];
    if (currentAcs.m_custodyIdFills.size() == 0) {
        return false;
    }
    serializedBundle.resize(CBHE_BPV6_MINIMUM_SAFE_PRIMARY_HEADER_ENCODE_SIZE + 2000); //todo size
    uint8_t * const serializationBase = &serializedBundle[0];
    uint8_t * buffer = serializationBase;

    bpv6_primary_block primary;
    memset(&primary, 0, sizeof(bpv6_primary_block));
    primary.version = 6;
    bpv6_canonical_block block;
    memset(&block, 0, sizeof(bpv6_canonical_block));

    primary.flags = bpv6_bundle_set_priority(bpv6_bundle_get_priority(primaryFromSender.flags)) |
        bpv6_bundle_set_gflags(BPV6_BUNDLEFLAG_SINGLETON | BPV6_BUNDLEFLAG_NOFRAGMENT | BPV6_BUNDLEFLAG_ADMIN_RECORD);
    primary.src_node = m_myCustodianNodeId;
    primary.src_svc = m_myCustodianServiceId;
    primary.dst_node = primaryFromSender.custodian_node;
    primary.dst_svc = primaryFromSender.custodian_svc;
    primary.custodian_node = 0;
    primary.custodian_svc = 0;
    primary.creation = 1000;//TODO //(uint64_t)bpv6_unix_to_5050(curr_time);
    primary.lifetime = 1000;
    primary.sequence = 1;
    uint64_t retVal;
    retVal = cbhe_bpv6_primary_block_encode(&primary, (char *)buffer, 0, 0);
    if (retVal == 0) {
        return false;
    }
    buffer += retVal;

    uint64_t sizeSerialized = currentAcs.Serialize(buffer);
    buffer += sizeSerialized;

    serializedBundle.resize(buffer - serializationBase);
    return true;
}

CustodyTransferManager::CustodyTransferManager(const bool isAcsAware, const uint64_t myCustodianNodeId, const uint64_t myCustodianServiceId) : 
    m_isAcsAware(isAcsAware),
    m_myCustodianNodeId(myCustodianNodeId),
    m_myCustodianServiceId(myCustodianServiceId),
    m_myCtebCreatorCustodianEidString(Uri::GetIpnUriString(m_myCustodianNodeId, m_myCustodianServiceId)),
    m_myNextCustodyIdAllocationBeginForNextHopCtebToSend(0)
{
    m_acsArray[static_cast<uint8_t>(BPV6_ACS_STATUS_REASON_INDICES::SUCCESS__NO_ADDITIONAL_INFORMATION)].SetCustodyTransferStatusAndReason(
        true, BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::NO_ADDITIONAL_INFORMATION);
    m_acsArray[static_cast<uint8_t>(BPV6_ACS_STATUS_REASON_INDICES::FAIL__REDUNDANT_RECEPTION)].SetCustodyTransferStatusAndReason(
        false, BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::REDUNDANT_RECEPTION);
    m_acsArray[static_cast<uint8_t>(BPV6_ACS_STATUS_REASON_INDICES::FAIL__DEPLETED_STORAGE)].SetCustodyTransferStatusAndReason(
        false, BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::DEPLETED_STORAGE);
    m_acsArray[static_cast<uint8_t>(BPV6_ACS_STATUS_REASON_INDICES::FAIL__DESTINATION_ENDPOINT_ID_UNINTELLIGIBLE)].SetCustodyTransferStatusAndReason(
        false, BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::DESTINATION_ENDPOINT_ID_UNINTELLIGIBLE);
    m_acsArray[static_cast<uint8_t>(BPV6_ACS_STATUS_REASON_INDICES::FAIL__NO_KNOWN_ROUTE_TO_DESTINATION_FROM_HERE)].SetCustodyTransferStatusAndReason(
        false, BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::NO_KNOWN_ROUTE_TO_DESTINATION_FROM_HERE);
    m_acsArray[static_cast<uint8_t>(BPV6_ACS_STATUS_REASON_INDICES::FAIL__NO_TIMELY_CONTACT_WITH_NEXT_NODE_ON_ROUTE)].SetCustodyTransferStatusAndReason(
        false, BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::NO_TIMELY_CONTACT_WITH_NEXT_NODE_ON_ROUTE);
    m_acsArray[static_cast<uint8_t>(BPV6_ACS_STATUS_REASON_INDICES::FAIL__BLOCK_UNINTELLIGIBLE)].SetCustodyTransferStatusAndReason(
        false, BPV6_CUSTODY_SIGNAL_REASON_CODES_7BIT::BLOCK_UNINTELLIGIBLE);
}
CustodyTransferManager::~CustodyTransferManager() {}



bool CustodyTransferManager::ProcessCustodyOfBundle(BundleViewV6 & bv, bool acceptCustody,
    const BPV6_ACS_STATUS_REASON_INDICES statusReasonIndex, std::vector<uint8_t> & custodySignalRfc5050SerializedBundle)
{
    custodySignalRfc5050SerializedBundle.resize(0);
    bpv6_primary_block & primary = bv.m_primaryBlockView.header;
    //bpv6_primary_block originalPrimaryFromSender = primary; //make a copy
    std::pair<uint64_t, uint64_t> custodianEidFromPrimary(primary.custodian_node, primary.custodian_svc);

    if (m_isAcsAware) {
        bool validCtebPresent = false;
        uint64_t receivedCtebCustodyId;
        std::vector<BundleViewV6::Bpv6CanonicalBlockView*> blocks;
        bv.GetCanonicalBlocksByType(BPV6_BLOCKTYPE_CUST_TRANSFER_EXT, blocks);
        if (blocks.size() > 1) { //D3.3.3 There shall be only one CTEB per bundle. 
            return false; //treat as malformed
        }
        else if (blocks.size() == 1) { //cteb present
            CustodyTransferEnhancementBlock cteb;
            uint32_t ctebLength = cteb.DeserializeCtebCanonicalBlock((const uint8_t*)blocks[0]->actualSerializedHeaderAndBodyPtr.data());


            uint64_t ctebNodeNumber;
            uint64_t ctebServiceNumber;
            if (!Uri::ParseIpnUriString(cteb.m_ctebCreatorCustodianEidString, ctebNodeNumber, ctebServiceNumber)) {
                return false;
            }
            std::pair<uint64_t, uint64_t> custodianEidFromCteb(ctebNodeNumber, ctebServiceNumber);
            //d) For an intermediate node which is ACS capable and accepts custody, the bundle
            //protocol agent compares the CTEB custodian with the primary bundle block
            //custodian.
            if (custodianEidFromPrimary == custodianEidFromCteb) {
                validCtebPresent = true;
                receivedCtebCustodyId = cteb.m_custodyId;
            }
            else {
                //If they are different, the CTEB is invalid and deleted.
                blocks[0]->markedForDeletion = true; //deleted when/if re-rendered
                //note.. unmark for deletion if going to reuse it if accept custody (below)
            }
            
        }
        if (acceptCustody) {
            if (validCtebPresent) { //identical custodians
                //acs capable ba, ba accepts custody, valid cteb => pending succeeded acs for custodian

                //d continued) For an intermediate node which is ACS capable and accepts custody...
                //For identical custodians, the primary bundle block and CTEB are updated with the new custodian
                //by the bundle protocol agent, and custody aggregation is utilized to improve link
                //efficiency
                //Item d) bounds the defining set of capabilities unique to ACS. By accepting a bundle for
                //custody transfer, an ACS - capable bundle protocol agent will process the bundle per RFC
                //5050, section 5.10.1.However, instead of the normal custody signaling, the CTEB identifies
                //in shortened form the specific bundle by custody ID.

                //REQUIREMENT D4.2.2.3c For ACS-aware bundle protocol agents which do accept custody of ACS:
                //  c) for bundles with a valid CTEB: 
                //      1) the bundle protocol agent shall aggregate �Succeeded� status into a single bundle
                //      as identified in D3.2:
                //          i) the aggregation of �Succeeded� status shall not exceed maximum allowed
                //          bundle size;
                //          ii) the time period for aggregation of bundle status shall not exceed the
                //          maximum allowed;
                //      2) the bundle protocol agent shall delete, upon successful transmission of an ACS
                //      signal, the associated timer and pending ACS �Succeeded�.

                //aggregate succeeded status
                m_acsArray[static_cast<uint8_t>(BPV6_ACS_STATUS_REASON_INDICES::SUCCESS__NO_ADDITIONAL_INFORMATION)].AddCustodyIdToFill(receivedCtebCustodyId);
            }
            else { //invalid cteb
                //acs capable ba, ba accepts custody, invalid cteb => generate succeeded and follow 5.10
                //invalid cteb was deleted above
                if (!GenerateCustodySignalBundle(custodySignalRfc5050SerializedBundle, primary, BPV6_ACS_STATUS_REASON_INDICES::SUCCESS__NO_ADDITIONAL_INFORMATION)) {
                    return false;
                }
            }

            //REQUIREMENT D4.2.2.3b For ACS-aware bundle protocol agents which do accept custody of ACS: (regardless of valid or invalid cteb)
            //  b) the bundle protocol agent shall update the custodian of the Primary Bundle Block and the CTEB as identified in D3.3;

            //update primary bundle block (pbb) with new custodian (do this after creating a custody signal to avoid copying the primary)
            primary.custodian_node = m_myCustodianNodeId;
            primary.custodian_svc = m_myCustodianServiceId;
            bv.m_primaryBlockView.SetManuallyModified(); //will update after render

            const uint64_t custodyId = GetNextCustodyIdForNextHopCtebToSend(cbhe_eid_t(primary.src_node, primary.src_svc));
            
            if (blocks.size() == 1) { //cteb present
                //update (reuse existing) CTEB with new custodian
                blocks[0]->markedForDeletion = false;
                std::vector<uint8_t> & newCtebBody = blocks[0]->replacementBlockBodyData;
                newCtebBody.resize(100);
                const uint64_t sizeSerialized = CustodyTransferEnhancementBlock::StaticSerializeCtebCanonicalBlockBody(&newCtebBody[0],
                    custodyId, m_myCtebCreatorCustodianEidString, blocks[0]->header);
                newCtebBody.resize(sizeSerialized);
                blocks[0]->SetManuallyModified(); //bundle needs rerendered
                //std::cout << "cteb present\n";
            }
            else { //non-existing cteb present.. append a new one to the bundle
                //https://cwe.ccsds.org/sis/docs/SIS-DTN/Meeting%20Materials/2011/Fall%20(Colorado)/jenkins-sisdtn-aggregate-custody-signals.pdf
                //slide 20 - ACS-enabled nodes add CTEBs when they become custodian.
                bpv6_canonical_block returnedCanonicalBlock;
                std::vector<uint8_t> newCtebBody(100);
                const uint64_t sizeSerialized = CustodyTransferEnhancementBlock::StaticSerializeCtebCanonicalBlockBody(&newCtebBody[0],
                    custodyId, m_myCtebCreatorCustodianEidString, returnedCanonicalBlock);
                newCtebBody.resize(sizeSerialized);
                bv.AppendCanonicalBlock(returnedCanonicalBlock, newCtebBody); //bundle needs rerendered
            }

            
        }
        else { //acs aware and don't accept custody
            //c) For an intermediate node which is ACS capable and does not accept custody,
            //possibility b) is the mode of operation.A node may not accept a bundle for any of a
            //number of reasons as defined in RFC 5050.

            
            //REQ D4.2.2.2 For ACS-aware bundle protocol agents which do not accept custody of ACS:
            if (validCtebPresent) { //identical custodians
                //acs capable ba, ba refuses custody, valid cteb => pending failed acs for custodian

                //b) for bundles with a valid CTEB:
                //  1) the bundle protocol agent shall aggregate �Failed� status into a single bundle as identified in D3.2:
                //      i) the aggregation of �Failed� status shall not exceed the maximum allowed bundle size;
                //      ii) the time period for aggregation of bundle status shall not exceed the maximum allowed;
                //  2) the bundle protocol agent shall transmit an ACS as identified in RFC 5050 section 5.10;
                //  3) the bundle protocol agent shall delete, upon successful transmission of an ACS
                //  signal, the associated timer and pending ACS �Failed�.

                //aggregate failed status
                m_acsArray[static_cast<uint8_t>(statusReasonIndex)].AddCustodyIdToFill(receivedCtebCustodyId);
            }
            else { //invalid cteb
                //acs capable ba, ba refuses custody, invalid cteb => generate failed and follow 5.10

                //a) for bundles without a valid CTEB block as identified in RFC 5050 section 5.10, the
                //  bundle protocol agent shall generate a �Failed� status;
                if (!GenerateCustodySignalBundle(custodySignalRfc5050SerializedBundle, primary, statusReasonIndex)) {
                    return false;
                }
            }
            

        }
    }
    else { //not acs aware
        //Req D4.2.2.1 Non-ACS-aware bundle protocol agents shall process ACS-supporting bundles per RFC 5050 section 5.10.
        if (acceptCustody) {
            //a) For an intermediate node which is not ACS capable and accepts custody, the bundle
            //protocol agent ignores the CTEB and updates the custodian field in the primary
            //bundle block.Since the CTEB custodian is not updated, the CTEB is invalid, and the
            //next ACS - capable bundle protocol agent will delete the CTEB.

            //acs unsupported ba, ba accepts custody => update pbb with custodian and generate succeeded and follow 5.10
                //invalid cteb was deleted above
            if (!GenerateCustodySignalBundle(custodySignalRfc5050SerializedBundle, primary, BPV6_ACS_STATUS_REASON_INDICES::SUCCESS__NO_ADDITIONAL_INFORMATION)) {
                return false;
            }

            //update primary bundle block (pbb) with new custodian (do this after creating a custody signal to avoid copying the primary)
            primary.custodian_node = m_myCustodianNodeId;
            primary.custodian_svc = m_myCustodianServiceId;
            bv.m_primaryBlockView.SetManuallyModified(); //will update after render
        }
        else {
            //b) For an intermediate node which is not ACS capable and does not accept custody, the
            //bundle protocol agent forwards the bundle without change.The CTEB is not
            //recognized.

            //acs unsupported ba, ba refuses custody => generate failed and follow 5.10

            if (!GenerateCustodySignalBundle(custodySignalRfc5050SerializedBundle, primary, statusReasonIndex)) {
                return false;
            }
        }
    }
    return true;
}

const AggregateCustodySignal & CustodyTransferManager::GetAcsConstRef(const BPV6_ACS_STATUS_REASON_INDICES statusReasonIndex) {
    return m_acsArray[static_cast<uint8_t>(statusReasonIndex)];
}

//bundle sources should have as much contiguous custody ids as possible
//in case of interleaving from multiple bundle sources,
//allocate integer range from [N*256+0, N*256+1, ... ,  N*256+255]
uint64_t CustodyTransferManager::GetNextCustodyIdForNextHopCtebToSend(const cbhe_eid_t & bundleSrcEid) {
    //uint64_t m_myNextCustodyIdAllocationBeginForNextHopCtebToSend;
    std::pair<std::map<cbhe_eid_t, uint64_t>::iterator, bool> res = m_mapBundleSrcEidToNextCtebCustodyId.insert(
        std::pair<cbhe_eid_t, uint64_t>(bundleSrcEid, m_myNextCustodyIdAllocationBeginForNextHopCtebToSend + 1)); //+1 because it's the next
    if (res.second == true) { //insertion due to first bundleSrcEid
        const uint64_t retVal = m_myNextCustodyIdAllocationBeginForNextHopCtebToSend;
        m_myNextCustodyIdAllocationBeginForNextHopCtebToSend += 256;
        return retVal;
    }
    else {
        //found but not inserted
        uint64_t & nextCtebCustodyId = res.first->second;
        const uint64_t retVal = nextCtebCustodyId;
        if ((++nextCtebCustodyId & 0xff) == 0) {
            nextCtebCustodyId = m_myNextCustodyIdAllocationBeginForNextHopCtebToSend;
            m_myNextCustodyIdAllocationBeginForNextHopCtebToSend += 256;
        }
        return retVal;
    }
}