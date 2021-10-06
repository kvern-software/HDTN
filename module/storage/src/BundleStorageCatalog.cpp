/***************************************************************************
 * NASA Glenn Research Center, Cleveland, OH
 * Released under the NASA Open Source Agreement (NOSA)
 * May  2021
 *
 ****************************************************************************
 */

#include "BundleStorageCatalog.h"
#include <iostream>
#include <string>
#include <boost/make_unique.hpp>


BundleStorageCatalog::BundleStorageCatalog() {}



BundleStorageCatalog::~BundleStorageCatalog() {}

bool BundleStorageCatalog::Insert_OrderBySequence(custids_flist_plus_lastiterator_t & custodyIdFlistPlusLastIt, const uint64_t custodyIdToInsert, const uint64_t mySequence) {
    custids_flist_t & custodyIdFlist = custodyIdFlistPlusLastIt.first;
    custids_flist_t::iterator & custodyIdFlistLastIterator = custodyIdFlistPlusLastIt.second;
    if (custodyIdFlist.empty()) {
        custodyIdFlist.emplace_front(custodyIdToInsert);
        custodyIdFlistLastIterator = custodyIdFlist.begin();
        return true;
    }
    else {
        
        //Chances are that bundle sequences will arrive in numerical order.
        //To optimize ordered insertion into an ordered singly-linked list, check if greater than the
        //last iterator first before iterating through the list.
        const uint64_t lastCustodyIdInList = *custodyIdFlistLastIterator;
        catalog_entry_t * lastEntryInList = m_custodyIdToCatalogEntryHashmap.GetValuePtr(lastCustodyIdInList);
        if (lastEntryInList == NULL) {
            return false; //unexpected error
        }
        const uint64_t lastSequenceInList = lastEntryInList->sequence;
        if (lastSequenceInList < mySequence) {
            //sequence is greater than every element in the bucket, insert at the end
            custodyIdFlistLastIterator = custodyIdFlist.emplace_after(custodyIdFlistLastIterator, custodyIdToInsert);
            return true;
        }
        

        //An out of order sequence arrived.  Revert to classic insertion
        custids_flist_t::const_iterator itPrev = custodyIdFlist.cbefore_begin();
        for (custids_flist_t::const_iterator it = custodyIdFlist.cbegin(); it != custodyIdFlist.cend(); ++it, ++itPrev) {
            const uint64_t custodyIdInList = *it;
            catalog_entry_t * entryInList = m_custodyIdToCatalogEntryHashmap.GetValuePtr(custodyIdInList);
            if (entryInList == NULL) {
                return false; //unexpected error
            }
            const uint64_t sequenceInList = entryInList->sequence;
            if (mySequence < sequenceInList) { //not in list, insert now
                break; //will call bucket.emplace_after(itPrev, custodyIdToInsert); and return true;
            }
            else if (sequenceInList < mySequence) { //greater than element
                continue;
            }
            else { //equal, already exists
                return false;
            }
        }
        //sequence is greater than every element iterated thus far in the bucket (but less than the last element), insert here
        custodyIdFlist.emplace_after(itPrev, custodyIdToInsert);
        return true;
    }
}
//won't detect duplicates
void BundleStorageCatalog::Insert_OrderByFifo(custids_flist_plus_lastiterator_t & custodyIdFlistPlusLastIt, const uint64_t custodyIdToInsert) {
    custids_flist_t & custodyIdFlist = custodyIdFlistPlusLastIt.first;
    custids_flist_t::iterator & custodyIdFlistLastIterator = custodyIdFlistPlusLastIt.second;
    if (custodyIdFlist.empty()) {
        custodyIdFlist.emplace_front(custodyIdToInsert);
        custodyIdFlistLastIterator = custodyIdFlist.begin();
    }
    else {
        custodyIdFlistLastIterator = custodyIdFlist.emplace_after(custodyIdFlistLastIterator, custodyIdToInsert);
    }
}
//won't detect duplicates
void BundleStorageCatalog::Insert_OrderByFilo(custids_flist_plus_lastiterator_t & custodyIdFlistPlusLastIt, const uint64_t custodyIdToInsert) {
    custids_flist_t & custodyIdFlist = custodyIdFlistPlusLastIt.first;
    custids_flist_t::iterator & custodyIdFlistLastIterator = custodyIdFlistPlusLastIt.second;
    const bool startedAsEmpty = custodyIdFlist.empty();
    custodyIdFlist.emplace_front(custodyIdToInsert);
    if (startedAsEmpty) {
        custodyIdFlistLastIterator = custodyIdFlist.begin();
    }
}

bool BundleStorageCatalog::Remove(custids_flist_plus_lastiterator_t & custodyIdFlistPlusLastIt, const uint64_t custodyIdToRemove) {
    custids_flist_t & custodyIdFlist = custodyIdFlistPlusLastIt.first;
    custids_flist_t::iterator & custodyIdFlistLastIterator = custodyIdFlistPlusLastIt.second;
    if (custodyIdFlist.empty()) {
        return false;
    }
    else {
        for (custids_flist_t::iterator itPrev = custodyIdFlist.before_begin(), it = custodyIdFlist.begin(); it != custodyIdFlist.end(); ++it, ++itPrev) {
            const uint64_t custodyIdInList = *it;
            if (custodyIdToRemove == custodyIdInList) { //equal, remove it
                if (it == custodyIdFlistLastIterator) {
                    custodyIdFlistLastIterator = itPrev;
                }
                custodyIdFlist.erase_after(itPrev);
                return true;
            }
        }
        //not found
        return false;
    }
}

bool BundleStorageCatalog::CatalogIncomingBundleForStore(catalog_entry_t & catalogEntryToTake, const bpv6_primary_block & primary, const uint64_t custodyId, const DUPLICATE_EXPIRY_ORDER order) {
    const uint32_t flags = bpv6_bundle_get_gflags(primary.flags);
    if (flags & BPV6_BUNDLEFLAG_CUSTODY) {
        if (flags & BPV6_BUNDLEFLAG_FRAGMENT) {
            const uuid_to_custid_hashmap_t::key_value_pair_t * p = m_uuidToCustodyIdHashMap.Insert(cbhe_bundle_uuid_t(primary), custodyId);
            if (p == NULL) {
                return false;
            }
            catalogEntryToTake.ptrUuidKeyInMap = &p->first;
        }
        else {
            const uuidnofrag_to_custid_hashmap_t::key_value_pair_t * p = m_uuidNoFragToCustodyIdHashMap.Insert(cbhe_bundle_uuid_nofragment_t(primary), custodyId);
            if (p == NULL) {
                return false;
            }
            catalogEntryToTake.ptrUuidKeyInMap = &p->first;
        }
    }
    if (!AddEntryToAwaitingSend(catalogEntryToTake, custodyId, order)) {
        return false;
    }
    if (!m_custodyIdToCatalogEntryHashmap.Insert(custodyId, std::move(catalogEntryToTake))) {
        return false;
    }
    
    return true;
}
bool BundleStorageCatalog::AddEntryToAwaitingSend(const catalog_entry_t & catalogEntry, const uint64_t custodyId, const DUPLICATE_EXPIRY_ORDER order) {
    priorities_to_expirations_array_t & priorityArray = m_destEidToPrioritiesMap[catalogEntry.destEid]; //created if not exist
    expirations_to_custids_map_t & expirationMap = priorityArray[catalogEntry.GetPriorityIndex()];
    custids_flist_plus_lastiterator_t & custodyIdFlistPlusLastIt = expirationMap[catalogEntry.GetAbsExpiration()];
    if (order == DUPLICATE_EXPIRY_ORDER::SEQUENCE_NUMBER) {
        return Insert_OrderBySequence(custodyIdFlistPlusLastIt, custodyId, catalogEntry.sequence);
    }
    else if (order == DUPLICATE_EXPIRY_ORDER::FIFO) {
        Insert_OrderByFifo(custodyIdFlistPlusLastIt, custodyId);
        return true;
    }
    else if (order == DUPLICATE_EXPIRY_ORDER::FILO) {
        Insert_OrderByFilo(custodyIdFlistPlusLastIt, custodyId);
        return true;
    }
    else {
        return false;
    }
}
bool BundleStorageCatalog::ReturnEntryToAwaitingSend(const catalog_entry_t & catalogEntry, const uint64_t custodyId) {
    //return what was popped off the front back to the front
    return AddEntryToAwaitingSend(catalogEntry, custodyId, DUPLICATE_EXPIRY_ORDER::FILO); 
}
bool BundleStorageCatalog::RemoveEntryFromAwaitingSend(const catalog_entry_t & catalogEntry, const uint64_t custodyId) {
    dest_eid_to_priorities_map_t::iterator destEidIt = m_destEidToPrioritiesMap.find(catalogEntry.destEid);
    if (destEidIt != m_destEidToPrioritiesMap.end()) {
        priorities_to_expirations_array_t & priorityArray = destEidIt->second; //created if not exist
        expirations_to_custids_map_t & expirationMap = priorityArray[catalogEntry.GetPriorityIndex()];
        expirations_to_custids_map_t::iterator expirationsIt = expirationMap.find(catalogEntry.GetAbsExpiration());
        if (expirationsIt != expirationMap.end()) {
            custids_flist_plus_lastiterator_t & custodyIdFlistPlusLastIt = expirationsIt->second;
            return Remove(custodyIdFlistPlusLastIt, custodyId);
        }
    }
    return false;
}
catalog_entry_t * BundleStorageCatalog::PopEntryFromAwaitingSend(uint64_t & custodyId, const std::vector<cbhe_eid_t> & availableDestEids) {
    std::vector<std::pair<const cbhe_eid_t*, priorities_to_expirations_array_t *> > destEidPlusPriorityArrayPtrs;
    destEidPlusPriorityArrayPtrs.reserve(availableDestEids.size());

    for (std::size_t i = 0; i < availableDestEids.size(); ++i) {
        const cbhe_eid_t & currentAvailableLink = availableDestEids[i];
        dest_eid_to_priorities_map_t::iterator dmIt = m_destEidToPrioritiesMap.find(currentAvailableLink);
        if (dmIt != m_destEidToPrioritiesMap.end()) {
            destEidPlusPriorityArrayPtrs.emplace_back(&currentAvailableLink, &(dmIt->second));
        }
    }

    //memset((uint8_t*)session.readCacheIsSegmentReady, 0, READ_CACHE_NUM_SEGMENTS_PER_SESSION);
    for (int i = NUMBER_OF_PRIORITIES - 1; i >= 0; --i) { //00 = bulk, 01 = normal, 10 = expedited
        uint64_t lowestExpiration = UINT64_MAX;
        expirations_to_custids_map_t * expirationMapPtr = NULL;
        custids_flist_plus_lastiterator_t * custIdFlistPlusLastItPtr = NULL;
        expirations_to_custids_map_t::iterator expirationMapIterator;

        for (std::size_t j = 0; j < destEidPlusPriorityArrayPtrs.size(); ++j) {
            priorities_to_expirations_array_t * priorityArray = destEidPlusPriorityArrayPtrs[j].second;
            //std::cout << "size " << (*priorityVec).size() << "\n";
            expirations_to_custids_map_t & expirationMap = (*priorityArray)[i];
            expirations_to_custids_map_t::iterator it = expirationMap.begin();
            if (it != expirationMap.end()) {
                const uint64_t thisExpiration = it->first;
                //std::cout << "thisexp " << thisExpiration << "\n";
                if (lowestExpiration > thisExpiration) {
                    lowestExpiration = thisExpiration;
                    expirationMapPtr = &expirationMap;
                    custIdFlistPlusLastItPtr = &it->second;
                    expirationMapIterator = it;
                }
            }
        }
        if (custIdFlistPlusLastItPtr) {
            custids_flist_t & cidFlist = custIdFlistPlusLastItPtr->first;
            custodyId = cidFlist.front();
            cidFlist.pop_front();

            if (cidFlist.empty()) {
                expirationMapPtr->erase(expirationMapIterator);
            }

            return m_custodyIdToCatalogEntryHashmap.GetValuePtr(custodyId);
        }
    }
    return NULL;
}



//return pair<success, numSuccessfulRemovals>
std::pair<bool, uint16_t> BundleStorageCatalog::Remove(const uint64_t custodyId, bool alsoNeedsRemovedFromAwaitingSend) {
    catalog_entry_t entry;
    bool error = false;
    uint16_t numRemovals = 0;
    if (!m_custodyIdToCatalogEntryHashmap.GetValueAndRemove(custodyId, entry)) {
        error = true;
    }
    else {
        ++numRemovals;
    }
    if ((!error) && alsoNeedsRemovedFromAwaitingSend) {
        if (!RemoveEntryFromAwaitingSend(entry, custodyId)) {
            error = true;
        }
        else {
            ++numRemovals;
        }
    }
    if (entry.HasCustodyAndFragmentation()) {
        uint64_t cidInMap;
        const cbhe_bundle_uuid_t* uuidPtr = (const cbhe_bundle_uuid_t*)entry.ptrUuidKeyInMap;
        if ((uuidPtr == NULL) || (!m_uuidToCustodyIdHashMap.GetValueAndRemove(*uuidPtr, cidInMap))) {
            error = true;
        }
        else {
            ++numRemovals;
        }
        if (cidInMap != custodyId) {
            error = true;
        }
    }
    if (entry.HasCustodyAndNonFragmentation()) {
        uint64_t cidInMap;
        const cbhe_bundle_uuid_nofragment_t* uuidPtr = (const cbhe_bundle_uuid_nofragment_t*)entry.ptrUuidKeyInMap;
        if ((uuidPtr == NULL) || (!m_uuidNoFragToCustodyIdHashMap.GetValueAndRemove(*uuidPtr, cidInMap))) {
            error = true;
        }
        else {
            ++numRemovals;
        }
        if (cidInMap != custodyId) {
            error = true;
        }
    }
    
    return std::pair<bool, uint16_t>(!error, numRemovals);
}
catalog_entry_t * BundleStorageCatalog::GetEntryFromCustodyId(const uint64_t custodyId) {
    return m_custodyIdToCatalogEntryHashmap.GetValuePtr(custodyId);
}
uint64_t * BundleStorageCatalog::GetCustodyIdFromUuid(const cbhe_bundle_uuid_t & bundleUuid) {
    return m_uuidToCustodyIdHashMap.GetValuePtr(bundleUuid);
}
uint64_t * BundleStorageCatalog::GetCustodyIdFromUuid(const cbhe_bundle_uuid_nofragment_t & bundleUuid) {
    return m_uuidNoFragToCustodyIdHashMap.GetValuePtr(bundleUuid);
}
