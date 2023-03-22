/**
 * @file LtpSessionSender.cpp
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

#include "LtpSessionSender.h"
#include "Logger.h"
#include <inttypes.h>
#include <boost/make_unique.hpp>
#include <boost/bind/bind.hpp>
#include <boost/next_prior.hpp>

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

void LtpSessionSender::LtpSessionSenderRecycledData::ClearAll() {
    m_dataFragmentsAckedByReceiver.clear();
    m_nonDataToSendFlistQueue.clear();
    m_resendFragmentsFlistQueue.clear();
    m_reportSegmentSerialNumbersReceivedSet.clear();
    m_checkpointSerialNumberActiveTimersList.clear();
    m_mapRsBoundsToRsnPendingGeneration.clear();
    //no need to clear m_tempListFragmentSetNeedingResentForEachReport
    //no need to clear m_tempFragmentsNeedingResent
}

LtpSessionSender::LtpSessionSenderCommonData::LtpSessionSenderCommonData(
    uint64_t mtuClientServiceData,
    uint64_t checkpointEveryNthDataPacket,
    uint32_t& maxRetriesPerSerialNumberRef,
    LtpTimerManager<Ltp::session_id_t, Ltp::hash_session_id_t>& timeManagerOfCheckpointSerialNumbersRef,
    const LtpTimerManager<Ltp::session_id_t, Ltp::hash_session_id_t>::LtpTimerExpiredCallback_t& csnTimerExpiredCallbackRef,
    LtpTimerManager<uint64_t, std::hash<uint64_t> >& timeManagerOfSendingDelayedDataSegmentsRef,
    const LtpTimerManager<uint64_t, std::hash<uint64_t> >::LtpTimerExpiredCallback_t& delayedDataSegmentsTimerExpiredCallbackRef,
    const NotifyEngineThatThisSenderNeedsDeletedCallback_t& notifyEngineThatThisSenderNeedsDeletedCallbackRef,
    const NotifyEngineThatThisSenderHasProducibleDataFunction_t& notifyEngineThatThisSenderHasProducibleDataFunctionRef,
    const InitialTransmissionCompletedCallback_t& initialTransmissionCompletedCallbackRef,
    LtpSessionSenderRecycler& ltpSessionSenderRecyclerRef) :
    //
    m_mtuClientServiceData(mtuClientServiceData),
    m_checkpointEveryNthDataPacket(checkpointEveryNthDataPacket),
    m_maxRetriesPerSerialNumberRef(maxRetriesPerSerialNumberRef),
    m_timeManagerOfCheckpointSerialNumbersRef(timeManagerOfCheckpointSerialNumbersRef),
    m_csnTimerExpiredCallbackRef(csnTimerExpiredCallbackRef),
    m_timeManagerOfSendingDelayedDataSegmentsRef(timeManagerOfSendingDelayedDataSegmentsRef),
    m_delayedDataSegmentsTimerExpiredCallbackRef(delayedDataSegmentsTimerExpiredCallbackRef),
    m_notifyEngineThatThisSenderNeedsDeletedCallbackRef(notifyEngineThatThisSenderNeedsDeletedCallbackRef),
    m_notifyEngineThatThisSenderHasProducibleDataFunctionRef(notifyEngineThatThisSenderHasProducibleDataFunctionRef),
    m_initialTransmissionCompletedCallbackRef(initialTransmissionCompletedCallbackRef),
    m_ltpSessionSenderRecyclerRef(ltpSessionSenderRecyclerRef),
    m_numCheckpointTimerExpiredCallbacks(0),
    m_numDiscretionaryCheckpointsNotResent(0),
    m_numDeletedFullyClaimedPendingReports(0) {}

LtpSessionSender::LtpSessionSender(uint64_t randomInitialSenderCheckpointSerialNumber, LtpClientServiceDataToSend&& dataToSend,
    std::shared_ptr<LtpTransmissionRequestUserData>&& userDataPtrToTake, uint64_t lengthOfRedPart,
    const Ltp::session_id_t& sessionId, const uint64_t clientServiceId,
    const uint64_t memoryBlockId, LtpSessionSenderCommonData& ltpSessionSenderCommonDataRef) :
    //
    m_largestEndIndexPendingGeneration(0),
    m_receptionClaimIndex(0),
    m_nextCheckpointSerialNumber(randomInitialSenderCheckpointSerialNumber),
    m_dataToSendSharedPtr(std::make_shared<LtpClientServiceDataToSend>(std::move(dataToSend))),
    m_userDataPtr(std::move(userDataPtrToTake)),
    M_LENGTH_OF_RED_PART(lengthOfRedPart),
    m_dataIndexFirstPass(0),
    M_SESSION_ID(sessionId),
    M_CLIENT_SERVICE_ID(clientServiceId),
    m_checkpointEveryNthDataPacketCounter(ltpSessionSenderCommonDataRef.m_checkpointEveryNthDataPacket),
    MEMORY_BLOCK_ID(memoryBlockId),
    m_ltpSessionSenderCommonDataRef(ltpSessionSenderCommonDataRef),
    //m_numActiveTimers(0),
    m_didNotifyForDeletion(false),
    m_allRedDataReceivedByRemote(false),
    m_isFailedSession(false),
    m_calledCancelledOrCompletedCallback(false)
{
    m_ltpSessionSenderCommonDataRef.m_ltpSessionSenderRecyclerRef.GetRecycledOrCreateNewUserData(m_ltpSessionSenderRecycledDataUniquePtr);
    if (m_ltpSessionSenderRecycledDataUniquePtr) {
        //successfully using recycled data
        m_ltpSessionSenderRecycledDataUniquePtr->ClearAll();
    }
    else {
        m_ltpSessionSenderRecycledDataUniquePtr = boost::make_unique<LtpSessionSenderRecycledData>();
    }
    
    //after creation by a transmission request, the transmission request function shall add this to a "first pass needing data sent" queue
    //m_notifyEngineThatThisSenderHasProducibleDataFunction(M_SESSION_ID.sessionNumber); //(old behavior) to trigger first pass of red data 
}

LtpSessionSender::~LtpSessionSender() {
    //clean up this sending session's active timers within the shared LtpTimerManager
    for (checkpoint_serial_number_active_timers_list_t::const_iterator it = m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.cbegin();
        it != m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.cend(); ++it)
    {
        const uint64_t csn = *it;

        // within a session would normally be LtpTimerManager<uint64_t, std::hash<uint64_t> > m_timeManagerOfCheckpointSerialNumbers;
        // but now sharing a single LtpTimerManager among all sessions, so use a
        // LtpTimerManager<Ltp::session_id_t, Ltp::hash_session_id_t> (which has hash map hashing function support)
        // such that: 
        //  sessionOriginatorEngineId = CHECKPOINT serial number
        //  sessionNumber = the session number
        //  since this is a sender, the real sessionOriginatorEngineId is constant among all sending sessions and is not needed
        const Ltp::session_id_t checkpointSerialNumberPlusSessionNumber(csn, M_SESSION_ID.sessionNumber);

        //This overload of DeleteTimer auto-recycles userData
        if (!m_ltpSessionSenderCommonDataRef.m_timeManagerOfCheckpointSerialNumbersRef.DeleteTimer(checkpointSerialNumberPlusSessionNumber)) {
            LOG_ERROR(subprocess) << "LtpSessionSender::~LtpSessionSender: did not delete timer";
        }
    }
    //clean up this sending session's single active timer within the shared LtpTimerManager
    if (m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration.size()) {
        //This overload of DeleteTimer auto-recycles userData (however userData not used in this timer so doesn't matter)
        if (!m_ltpSessionSenderCommonDataRef.m_timeManagerOfSendingDelayedDataSegmentsRef.DeleteTimer(M_SESSION_ID.sessionNumber)) {
            LOG_ERROR(subprocess) << "LtpSessionSender::~LtpSessionSender: did not delete timer in m_timeManagerOfSendingDelayedDataSegmentsRef";
        }
    }
    //recycle the data structures with custom allocators 
    m_ltpSessionSenderCommonDataRef.m_ltpSessionSenderRecyclerRef.ReturnUserData(std::move(m_ltpSessionSenderRecycledDataUniquePtr));
}

void LtpSessionSender::LtpCheckpointTimerExpiredCallback(const Ltp::session_id_t& checkpointSerialNumberPlusSessionNumber, std::vector<uint8_t> & userData) {
    // within a session would normally be LtpTimerManager<uint64_t, std::hash<uint64_t> > m_timeManagerOfCheckpointSerialNumbers;
    // but now sharing a single LtpTimerManager among all sessions, so use a
    // LtpTimerManager<Ltp::session_id_t, Ltp::hash_session_id_t> (which has hash map hashing function support)
    // such that: 
    //  sessionOriginatorEngineId = CHECKPOINT serial number
    //  sessionNumber = the session number
    //  since this is a sender, the real sessionOriginatorEngineId is constant among all sending sessions and is not needed
    const uint64_t checkpointSerialNumber = checkpointSerialNumberPlusSessionNumber.sessionOriginatorEngineId;

    if (userData.size() != sizeof(csntimer_userdata_t)) {
        LOG_ERROR(subprocess) << "LtpSessionReceiver::LtpReportSegmentTimerExpiredCallback: userData.size() != sizeof(rsntimer_userdata_t)";
        return;
    }
    csntimer_userdata_t* userDataPtr = reinterpret_cast<csntimer_userdata_t*>(userData.data());

    //keep track of this sending session's active timers within the shared LtpTimerManager
    m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.erase(userDataPtr->itCheckpointSerialNumberActiveTimersList);

    //6.7.  Retransmit Checkpoint
    //This procedure is triggered by the expiration of a countdown timer
    //associated with a CP segment.
    //
    //Response: if the number of times this CP segment has been queued for
    //transmission exceeds the checkpoint retransmission limit established
    //for the local LTP engine by network management, then the session of
    //which the segment is one token is canceled : the "Cancel Session"
    //procedure(Section 6.19) is invoked, a CS with reason - code RLEXC is
    //appended to the(conceptual) application data queue, and a
    //transmission - session cancellation notice(Section 7.5) is sent back
    //to the client service that requested the transmission.
    //
    //Otherwise, a new copy of the CP segment is appended to the
    //(conceptual) application data queue for the destination LTP engine.

    
    ++m_ltpSessionSenderCommonDataRef.m_numCheckpointTimerExpiredCallbacks;

    resend_fragment_t & resendFragment = userDataPtr->resendFragment;

    if (resendFragment.retryCount <= m_ltpSessionSenderCommonDataRef.m_maxRetriesPerSerialNumberRef) {
        const bool isDiscretionaryCheckpoint = (resendFragment.flags == LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT);
        if (isDiscretionaryCheckpoint && LtpFragmentSet::ContainsFragmentEntirely(m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver,
            LtpFragmentSet::data_fragment_t(resendFragment.offset, (resendFragment.offset + resendFragment.length) - 1)))
        {
            ++m_ltpSessionSenderCommonDataRef.m_numDiscretionaryCheckpointsNotResent;
        }
        else {
            //resend 
            ++(resendFragment.retryCount);
            m_ltpSessionSenderRecycledDataUniquePtr->m_resendFragmentsFlistQueue.push_back(resendFragment);
            m_ltpSessionSenderCommonDataRef.m_notifyEngineThatThisSenderHasProducibleDataFunctionRef(M_SESSION_ID.sessionNumber);
        }
    }
    else {
        if (!m_didNotifyForDeletion) {
            m_isFailedSession = true;
            m_didNotifyForDeletion = true;
            m_ltpSessionSenderCommonDataRef.m_notifyEngineThatThisSenderNeedsDeletedCallbackRef(M_SESSION_ID, true, CANCEL_SEGMENT_REASON_CODES::RLEXC, m_userDataPtr);
        }
    }
    //userData shall be recycled automatically after this callback completes
}

void LtpSessionSender::LtpDelaySendDataSegmentsTimerExpiredCallback(const uint64_t& sessionNumber, std::vector<uint8_t>& userData) {
    // Github issue 24: Defer data retransmission with out-of-order report segments (see detailed description below)
    //...When the retransmission timer expires (i.e. there are still gaps to send) then send data segments to cover the remaining gaps for the session.
    LtpFragmentSet::list_fragment_set_needing_resent_for_each_report_t& listFragmentSetNeedingResentForEachReport = 
        m_ltpSessionSenderRecycledDataUniquePtr->m_tempListFragmentSetNeedingResentForEachReport;
    //list gets cleared with call to ReduceReportSegments
    LtpFragmentSet::ReduceReportSegments(m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration,
        m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver, listFragmentSetNeedingResentForEachReport);
    for (LtpFragmentSet::list_fragment_set_needing_resent_for_each_report_t::const_iterator it = listFragmentSetNeedingResentForEachReport.cbegin();
        it != listFragmentSetNeedingResentForEachReport.cend(); ++it)
    {
        ResendDataFromReport(it->second, it->first);
    }
    m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration.clear(); //also flag that signifies timer stopped
    if (!m_didNotifyForDeletion) {
        m_ltpSessionSenderCommonDataRef.m_notifyEngineThatThisSenderHasProducibleDataFunctionRef(M_SESSION_ID.sessionNumber);
    }
    //userData shall be recycled automatically after this callback completes (however it is unused and empty, so nothing to recycle)
}

bool LtpSessionSender::NextTimeCriticalDataToSend(UdpSendPacketInfo& udpSendPacketInfo) {
    if (!m_ltpSessionSenderRecycledDataUniquePtr->m_nonDataToSendFlistQueue.empty()) { //includes report ack segments
        //highest priority
        if (udpSendPacketInfo.underlyingDataToDeleteOnSentCallback && (udpSendPacketInfo.underlyingDataToDeleteOnSentCallback->size() >= 1)) {
            //no need to invoke operator new since it was preallocated
        }
        else {
            udpSendPacketInfo.underlyingDataToDeleteOnSentCallback = std::make_shared<std::vector<std::vector<uint8_t> > >(1);
        }
        (*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[0] = std::move(m_ltpSessionSenderRecycledDataUniquePtr->m_nonDataToSendFlistQueue.front());
        m_ltpSessionSenderRecycledDataUniquePtr->m_nonDataToSendFlistQueue.pop();
        udpSendPacketInfo.constBufferVec.resize(1);
        udpSendPacketInfo.constBufferVec[0] = boost::asio::buffer((*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[0]);
        return true;
    }

    while (!m_ltpSessionSenderRecycledDataUniquePtr->m_resendFragmentsFlistQueue.empty()) {
        if (m_allRedDataReceivedByRemote) {
            //Continuation of Github issue 23:
            //If the sender detects that all Red data has been acknowledged by the remote,
            //the sender shall remove all Red data segments (checkpoint or non-checkpoint) from the
            //outgoing transmission queue.
            m_ltpSessionSenderRecycledDataUniquePtr->m_resendFragmentsFlistQueue.pop();
            continue;
        }
        LtpSessionSender::resend_fragment_t& resendFragment = m_ltpSessionSenderRecycledDataUniquePtr->m_resendFragmentsFlistQueue.front();
        Ltp::data_segment_metadata_t meta;
        meta.clientServiceId = M_CLIENT_SERVICE_ID;
        meta.offset = resendFragment.offset;
        meta.length = resendFragment.length;
        const bool isCheckpoint = (resendFragment.flags != LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA);
        if (isCheckpoint) {
            meta.checkpointSerialNumber = &resendFragment.checkpointSerialNumber;
            meta.reportSerialNumber = &resendFragment.reportSerialNumber;

            //6.2.  Start Checkpoint Timer
            //This procedure is triggered by the arrival of a link state cue
            //indicating the de - queuing(for transmission) of a CP segment.
            //
            //Response: the expected arrival time of the RS segment that will be
            //produced on reception of this CP segment is computed, and a countdown
            //timer is started for this arrival time.However, if it is known that
            //the remote LTP engine has ceased transmission(Section 6.5), then
            //this timer is immediately suspended, because the computed expected
            //arrival time may require an adjustment that cannot yet be computed.


            // within a session would normally be LtpTimerManager<uint64_t, std::hash<uint64_t> > m_timeManagerOfCheckpointSerialNumbers;
            // but now sharing a single LtpTimerManager among all sessions, so use a
            // LtpTimerManager<Ltp::session_id_t, Ltp::hash_session_id_t> (which has hash map hashing function support)
            // such that: 
            //  sessionOriginatorEngineId = CHECKPOINT serial number
            //  sessionNumber = the session number
            //  since this is a sender, the real sessionOriginatorEngineId is constant among all sending sessions and is not needed
            const Ltp::session_id_t checkpointSerialNumberPlusSessionNumber(resendFragment.checkpointSerialNumber, M_SESSION_ID.sessionNumber);
            std::vector<uint8_t> userData;
            m_ltpSessionSenderCommonDataRef.m_timeManagerOfCheckpointSerialNumbersRef.m_userDataRecycler.GetRecycledOrCreateNewUserData(userData);
            userData.resize(sizeof(csntimer_userdata_t));
            csntimer_userdata_t* userDataPtr = reinterpret_cast<csntimer_userdata_t*>(userData.data());
            userDataPtr->resendFragment = resendFragment;
            m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.emplace_front(resendFragment.checkpointSerialNumber); //keep track of this sending session's active timers within the shared LtpTimerManager
            userDataPtr->itCheckpointSerialNumberActiveTimersList = m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.begin();
            if (!m_ltpSessionSenderCommonDataRef.m_timeManagerOfCheckpointSerialNumbersRef.StartTimer(
                this, checkpointSerialNumberPlusSessionNumber, &m_ltpSessionSenderCommonDataRef.m_csnTimerExpiredCallbackRef, std::move(userData)))
            {
                m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.erase(
                    m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.begin());
                LOG_ERROR(subprocess) << "LtpSessionSender::NextDataToSend: did not start timer";
            }
        }
        else { //non-checkpoint
            meta.checkpointSerialNumber = NULL;
            meta.reportSerialNumber = NULL;
        }
        const bool needsToReadClientServiceDataFromDisk = (m_dataToSendSharedPtr->data() == NULL);
        const std::size_t neededUnderlyingSize = 1 + needsToReadClientServiceDataFromDisk; //2 would be needed in case of trailer extensions (but not used here)
        if (udpSendPacketInfo.underlyingDataToDeleteOnSentCallback && (udpSendPacketInfo.underlyingDataToDeleteOnSentCallback->size() >= neededUnderlyingSize)) {
            //no need to invoke operator new since it was preallocated
        }
        else {
            udpSendPacketInfo.underlyingDataToDeleteOnSentCallback = std::make_shared<std::vector<std::vector<uint8_t> > >(neededUnderlyingSize);
        }
        Ltp::GenerateLtpHeaderPlusDataSegmentMetadata((*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[0], resendFragment.flags,
            M_SESSION_ID, meta, NULL, 0);
        udpSendPacketInfo.constBufferVec.resize(2); //3 would be needed in case of trailer extensions (but not used here)
        udpSendPacketInfo.constBufferVec[0] = boost::asio::buffer((*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[0]);
        if (needsToReadClientServiceDataFromDisk) {
            std::vector<uint8_t>& readLocation = (*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[1];
            readLocation.resize(resendFragment.length);
            udpSendPacketInfo.constBufferVec[1] = boost::asio::buffer(readLocation.data(), resendFragment.length);
            udpSendPacketInfo.deferredRead.memoryBlockId = MEMORY_BLOCK_ID;
            udpSendPacketInfo.deferredRead.length = static_cast<std::size_t>(resendFragment.length);
            udpSendPacketInfo.deferredRead.offset = resendFragment.offset;
            udpSendPacketInfo.deferredRead.readToThisLocationPtr = readLocation.data();
        }
        else {
            udpSendPacketInfo.constBufferVec[1] = boost::asio::buffer(m_dataToSendSharedPtr->data() + resendFragment.offset, resendFragment.length);
        }
        
        //Increase the reference count of the LtpClientServiceDataToSend shared_ptr
        //so that the LtpClientServiceDataToSend won't get deleted before the UDP send operation completes.
        //This event would be caused by the LtpSessionSender getting deleted before the UDP send operation completes,
        //which would almost always happen with green data and could happen with red data.
        if (!needsToReadClientServiceDataFromDisk) {
            udpSendPacketInfo.underlyingCsDataToDeleteOnSentCallback = m_dataToSendSharedPtr;
        }
        m_ltpSessionSenderRecycledDataUniquePtr->m_resendFragmentsFlistQueue.pop();
        return true;
    }

    return false;
}

bool LtpSessionSender::NextFirstPassDataToSend(UdpSendPacketInfo& udpSendPacketInfo) {
    if (m_dataIndexFirstPass < m_dataToSendSharedPtr->size()) {
        const bool needsToReadClientServiceDataFromDisk = (m_dataToSendSharedPtr->data() == NULL);
        if (m_dataIndexFirstPass < M_LENGTH_OF_RED_PART) { //first pass of red data send
            const uint64_t bytesToSendRed = std::min(M_LENGTH_OF_RED_PART - m_dataIndexFirstPass, m_ltpSessionSenderCommonDataRef.m_mtuClientServiceData);
            const bool isEndOfRedPart = ((bytesToSendRed + m_dataIndexFirstPass) == M_LENGTH_OF_RED_PART);
            bool isPeriodicCheckpoint = false;
            if (m_ltpSessionSenderCommonDataRef.m_checkpointEveryNthDataPacket && (--m_checkpointEveryNthDataPacketCounter == 0)) {
                m_checkpointEveryNthDataPacketCounter = m_ltpSessionSenderCommonDataRef.m_checkpointEveryNthDataPacket;
                isPeriodicCheckpoint = true;
            }
            const bool isCheckpoint = isPeriodicCheckpoint || isEndOfRedPart;

            LTP_DATA_SEGMENT_TYPE_FLAGS flags = LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA;
            uint64_t cp;
            uint64_t * checkpointSerialNumber = NULL;
            uint64_t rsn = 0; //0 in this state because not a response to an RS
            uint64_t * reportSerialNumber = NULL;
            if (isCheckpoint) {
                flags = LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT;
                cp = m_nextCheckpointSerialNumber++;
                checkpointSerialNumber = &cp;
                reportSerialNumber = &rsn;
                if (isEndOfRedPart) {
                    flags = LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART;
                    const bool isEndOfBlock = (M_LENGTH_OF_RED_PART == m_dataToSendSharedPtr->size());
                    if (isEndOfBlock) {
                        flags = LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART_ENDOFBLOCK;
                    }
                }
                std::vector<uint8_t> userData;
                m_ltpSessionSenderCommonDataRef.m_timeManagerOfCheckpointSerialNumbersRef.m_userDataRecycler.GetRecycledOrCreateNewUserData(userData);
                userData.resize(sizeof(csntimer_userdata_t));
                csntimer_userdata_t* userDataPtr = reinterpret_cast<csntimer_userdata_t*>(userData.data());
                LtpSessionSender::resend_fragment_t & resendFragment = userDataPtr->resendFragment;
                new (&resendFragment) LtpSessionSender::resend_fragment_t(m_dataIndexFirstPass, bytesToSendRed, cp, rsn, flags); //placement new

                // within a session would normally be LtpTimerManager<uint64_t, std::hash<uint64_t> > m_timeManagerOfCheckpointSerialNumbers;
                // but now sharing a single LtpTimerManager among all sessions, so use a
                // LtpTimerManager<Ltp::session_id_t, Ltp::hash_session_id_t> (which has hash map hashing function support)
                // such that: 
                //  sessionOriginatorEngineId = CHECKPOINT serial number
                //  sessionNumber = the session number
                //  since this is a sender, the real sessionOriginatorEngineId is constant among all sending sessions and is not needed
                const Ltp::session_id_t checkpointSerialNumberPlusSessionNumber(cp, M_SESSION_ID.sessionNumber);

                m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.emplace_front(cp); //keep track of this sending session's active timers within the shared LtpTimerManager
                userDataPtr->itCheckpointSerialNumberActiveTimersList = m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.begin();
                if (!m_ltpSessionSenderCommonDataRef.m_timeManagerOfCheckpointSerialNumbersRef.StartTimer(
                    this, checkpointSerialNumberPlusSessionNumber, &m_ltpSessionSenderCommonDataRef.m_csnTimerExpiredCallbackRef, std::move(userData)))
                {
                    m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.erase(
                        m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.begin());
                    LOG_ERROR(subprocess) << "LtpSessionSender::NextDataToSend: did not start timer";
                }
            }

            Ltp::data_segment_metadata_t meta;
            meta.clientServiceId = M_CLIENT_SERVICE_ID;
            meta.offset = m_dataIndexFirstPass;
            meta.length = bytesToSendRed;
            meta.checkpointSerialNumber = checkpointSerialNumber;
            meta.reportSerialNumber = reportSerialNumber;
            const std::size_t neededUnderlyingSize = 1 + needsToReadClientServiceDataFromDisk; //2 would be needed in case of trailer extensions (but not used here)
            if (udpSendPacketInfo.underlyingDataToDeleteOnSentCallback && (udpSendPacketInfo.underlyingDataToDeleteOnSentCallback->size() >= neededUnderlyingSize)) {
                //no need to invoke operator new since it was preallocated
            }
            else {
                udpSendPacketInfo.underlyingDataToDeleteOnSentCallback = std::make_shared<std::vector<std::vector<uint8_t> > >(neededUnderlyingSize);
            }
            Ltp::GenerateLtpHeaderPlusDataSegmentMetadata((*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[0], flags,
                M_SESSION_ID, meta, NULL, 0);
            udpSendPacketInfo.constBufferVec.resize(2); //3 would be needed in case of trailer extensions (but not used here)
            udpSendPacketInfo.constBufferVec[0] = boost::asio::buffer((*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[0]);
            if (needsToReadClientServiceDataFromDisk) {
                std::vector<uint8_t>& readLocation = (*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[1];
                readLocation.resize(bytesToSendRed);
                udpSendPacketInfo.constBufferVec[1] = boost::asio::buffer(readLocation);
                udpSendPacketInfo.deferredRead.memoryBlockId = MEMORY_BLOCK_ID;
                udpSendPacketInfo.deferredRead.length = static_cast<std::size_t>(bytesToSendRed);
                udpSendPacketInfo.deferredRead.offset = m_dataIndexFirstPass;
                udpSendPacketInfo.deferredRead.readToThisLocationPtr = readLocation.data();
            }
            else {
                udpSendPacketInfo.constBufferVec[1] = boost::asio::buffer(m_dataToSendSharedPtr->data() + m_dataIndexFirstPass, bytesToSendRed);
            }
            m_dataIndexFirstPass += bytesToSendRed;
        }
        else { //first pass of green data send
            uint64_t bytesToSendGreen = std::min(m_dataToSendSharedPtr->size() - m_dataIndexFirstPass, m_ltpSessionSenderCommonDataRef.m_mtuClientServiceData);
            const bool isEndOfBlock = ((bytesToSendGreen + m_dataIndexFirstPass) == m_dataToSendSharedPtr->size());
            LTP_DATA_SEGMENT_TYPE_FLAGS flags = LTP_DATA_SEGMENT_TYPE_FLAGS::GREENDATA;
            if (isEndOfBlock) {
                flags = LTP_DATA_SEGMENT_TYPE_FLAGS::GREENDATA_ENDOFBLOCK;
            }
            Ltp::data_segment_metadata_t meta;
            meta.clientServiceId = M_CLIENT_SERVICE_ID;
            meta.offset = m_dataIndexFirstPass;
            meta.length = bytesToSendGreen;
            meta.checkpointSerialNumber = NULL;
            meta.reportSerialNumber = NULL;
            const std::size_t neededUnderlyingSize = 1 + needsToReadClientServiceDataFromDisk; //2 would be needed in case of trailer extensions (but not used here)
            if (udpSendPacketInfo.underlyingDataToDeleteOnSentCallback && (udpSendPacketInfo.underlyingDataToDeleteOnSentCallback->size() >= neededUnderlyingSize)) {
                //no need to invoke operator new since it was preallocated
            }
            else {
                udpSendPacketInfo.underlyingDataToDeleteOnSentCallback = std::make_shared<std::vector<std::vector<uint8_t> > >(neededUnderlyingSize);
            }
            Ltp::GenerateLtpHeaderPlusDataSegmentMetadata((*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[0], flags,
                M_SESSION_ID, meta, NULL, 0);
            udpSendPacketInfo.constBufferVec.resize(2); //3 would be needed in case of trailer extensions (but not used here)
            udpSendPacketInfo.constBufferVec[0] = boost::asio::buffer((*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[0]);
            if (needsToReadClientServiceDataFromDisk) {
                std::vector<uint8_t>& readLocation = (*udpSendPacketInfo.underlyingDataToDeleteOnSentCallback)[1];
                readLocation.resize(bytesToSendGreen);
                udpSendPacketInfo.constBufferVec[1] = boost::asio::buffer(readLocation);
                udpSendPacketInfo.deferredRead.memoryBlockId = MEMORY_BLOCK_ID;
                udpSendPacketInfo.deferredRead.length = static_cast<std::size_t>(bytesToSendGreen);
                udpSendPacketInfo.deferredRead.offset = m_dataIndexFirstPass;
                udpSendPacketInfo.deferredRead.readToThisLocationPtr = readLocation.data();
            }
            else {
                udpSendPacketInfo.constBufferVec[1] = boost::asio::buffer(m_dataToSendSharedPtr->data() + m_dataIndexFirstPass, bytesToSendGreen);
            }
            m_dataIndexFirstPass += bytesToSendGreen;
        }
        if (m_dataIndexFirstPass == m_dataToSendSharedPtr->size()) { //only ever enters here once
            //Increase the reference count of the LtpClientServiceDataToSend shared_ptr
            //so that the LtpClientServiceDataToSend won't get deleted before the UDP send operation completes.
            //This event would be caused by the LtpSessionSender getting deleted before the UDP send operation completes,
            //which would almost always happen with green data and could happen with red data.
            if (!needsToReadClientServiceDataFromDisk) {
                udpSendPacketInfo.underlyingCsDataToDeleteOnSentCallback = m_dataToSendSharedPtr;
            }

            m_ltpSessionSenderCommonDataRef.m_initialTransmissionCompletedCallbackRef(M_SESSION_ID, m_userDataPtr);
            if (M_LENGTH_OF_RED_PART == 0) { //fully green case complete (notify engine for deletion)
                if (!m_didNotifyForDeletion) {
                    m_didNotifyForDeletion = true;
                    m_ltpSessionSenderCommonDataRef.m_notifyEngineThatThisSenderNeedsDeletedCallbackRef(M_SESSION_ID, false, CANCEL_SEGMENT_REASON_CODES::RESERVED, m_userDataPtr);
                }
            }
            else if (m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver.size() == 1) { //in case red data already acked before green data send completes
                LtpFragmentSet::data_fragment_set_t::const_iterator it = m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver.cbegin();
                if ((it->beginIndex == 0) && (it->endIndex >= (M_LENGTH_OF_RED_PART - 1))) { //>= in case some green data was acked
                    if (!m_didNotifyForDeletion) {
                        m_didNotifyForDeletion = true;
                        m_ltpSessionSenderCommonDataRef.m_notifyEngineThatThisSenderNeedsDeletedCallbackRef(M_SESSION_ID, false, CANCEL_SEGMENT_REASON_CODES::RESERVED, m_userDataPtr);
                    }
                }
            }
        }
        
        return true;
    }
    return false;
}


void LtpSessionSender::ReportSegmentReceivedCallback(const Ltp::report_segment_t & reportSegment,
    Ltp::ltp_extensions_t & headerExtensions, Ltp::ltp_extensions_t & trailerExtensions)
{
    //6.13.  Retransmit Data

    //Response: first, an RA segment with the same report serial number as
    //the RS segment is issued and is, in concept, appended to the queue of
    //internal operations traffic bound for the receiver.
    m_ltpSessionSenderRecycledDataUniquePtr->m_nonDataToSendFlistQueue.emplace_back(); //m_notifyEngineThatThisSenderHasProducibleDataFunction at the end of this function
    Ltp::GenerateReportAcknowledgementSegmentLtpPacket(m_ltpSessionSenderRecycledDataUniquePtr->m_nonDataToSendFlistQueue.back(),
        M_SESSION_ID, reportSegment.reportSerialNumber, NULL, NULL);

    //If the RS segment is redundant -- i.e., either the indicated session is unknown
    //(for example, the RS segment is received after the session has been
    //completed or canceled) or the RS segment's report serial number
    //matches that of an RS segment that has already been received and
    //processed -- then no further action is taken.
    if (m_ltpSessionSenderRecycledDataUniquePtr->m_reportSegmentSerialNumbersReceivedSet.insert(reportSegment.reportSerialNumber).second) { //serial number was inserted (it's new)
        //If the report's checkpoint serial number is not zero, then the
        //countdown timer associated with the indicated checkpoint segment is deleted.
        if (reportSegment.checkpointSerialNumber) {

            // within a session would normally be LtpTimerManager<uint64_t, std::hash<uint64_t> > m_timeManagerOfCheckpointSerialNumbers;
            // but now sharing a single LtpTimerManager among all sessions, so use a
            // LtpTimerManager<Ltp::session_id_t, Ltp::hash_session_id_t> (which has hash map hashing function support)
            // such that: 
            //  sessionOriginatorEngineId = CHECKPOINT serial number
            //  sessionNumber = the session number
            //  since this is a sender, the real sessionOriginatorEngineId is constant among all sending sessions and is not needed
            const Ltp::session_id_t checkpointSerialNumberPlusSessionNumber(reportSegment.checkpointSerialNumber, M_SESSION_ID.sessionNumber);

            std::vector<uint8_t> userDataReturned;
            if (m_ltpSessionSenderCommonDataRef.m_timeManagerOfCheckpointSerialNumbersRef.DeleteTimer(checkpointSerialNumberPlusSessionNumber, userDataReturned)) { //if delete of a timer was successful
                if (userDataReturned.size() != sizeof(csntimer_userdata_t)) {
                    LOG_ERROR(subprocess) << "LtpSessionSender::ReportSegmentReceivedCallback: userDataReturned.size() != sizeof(csntimer_userdata_t)";
                }
                else {
                    const csntimer_userdata_t* userDataPtr = reinterpret_cast<csntimer_userdata_t*>(userDataReturned.data());
                    //keep track of this sending session's active timers within the shared LtpTimerManager
                    m_ltpSessionSenderRecycledDataUniquePtr->m_checkpointSerialNumberActiveTimersList.erase(userDataPtr->itCheckpointSerialNumberActiveTimersList);
                    //this overload of DeleteTimer does not auto-recycle user data and must be manually invoked
                    m_ltpSessionSenderCommonDataRef.m_timeManagerOfCheckpointSerialNumbersRef.m_userDataRecycler.ReturnUserData(std::move(userDataReturned));
                }
            }
        }


        if (LtpFragmentSet::AddReportSegmentToFragmentSet(m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver, reportSegment)) { //this RS indicates new acks by receiver

            

            //6.12.  Signify Transmission Completion
            //
            //This procedure is triggered at the earliest time at which(a) all
            //data in the block are known to have been transmitted *and* (b)the
            //entire red - part of the block-- if of non - zero length -- is known to
            //have been successfully received.Condition(a) is signaled by
            //arrival of a link state cue indicating the de - queuing(for
            //transmission) of the EOB segment for the block.Condition(b) is
            //signaled by reception of an RS segment whose reception claims, taken
            //together with the reception claims of all other RS segments
            //previously received in the course of this session, indicate complete
            //reception of the red - part of the block.
            //
            //Response: a transmission - session completion notice(Section 7.4) is
            //sent to the local client service associated with the session, and the
            //session is closed : the "Close Session" procedure(Section 6.20) is
            //invoked.
            if (m_allRedDataReceivedByRemote == false) { //the m_allRedDataReceivedByRemote flag is used to prevent resending of non-checkpoint data (Continuation of Github issue 23)
                if (m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver.size() == 1) {
                    LtpFragmentSet::data_fragment_set_t::const_iterator it = m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver.cbegin();
                    if ((it->beginIndex == 0) && (it->endIndex >= (M_LENGTH_OF_RED_PART - 1))) { //>= in case some green data was acked
                        m_allRedDataReceivedByRemote = true;
                    }
                }
            }
            if ((m_dataIndexFirstPass == m_dataToSendSharedPtr->size()) && m_allRedDataReceivedByRemote) { //if red and green fully sent and all red data acked
                if (!m_didNotifyForDeletion) {
                    m_didNotifyForDeletion = true;
                    m_ltpSessionSenderCommonDataRef.m_notifyEngineThatThisSenderNeedsDeletedCallbackRef(M_SESSION_ID, false, CANCEL_SEGMENT_REASON_CODES::RESERVED, m_userDataPtr);
                }
            }


            //If the segment's reception claims indicate incomplete data reception
            //within the scope of the report segment :
            //If the number of transmission problems for this session has not
            //exceeded any limit, new data segments encapsulating all block
            //data whose non - reception is implied by the reception claims are
            //appended to the transmission queue bound for the receiver.The
            //last-- and only the last -- data segment must be marked as a CP
            //segment carrying a new CP serial number(obtained by
            //incrementing the last CP serial number used) and the report
            //serial number of the received RS segment.
            LtpFragmentSet::data_fragment_set_t& fragmentsNeedingResent = m_ltpSessionSenderRecycledDataUniquePtr->m_tempFragmentsNeedingResent;
#if 0
            fragmentsNeedingResent.clear();
            LtpFragmentSet::AddReportSegmentToFragmentSetNeedingResent(fragmentsNeedingResent, reportSegment);
#else
            // improvements from Github issue 24
            const FragmentSet::data_fragment_t bounds(reportSegment.lowerBound, reportSegment.upperBound - 1);
            const LtpFragmentSet::data_fragment_unique_overlapping_t& boundsUnique = *(reinterpret_cast<const LtpFragmentSet::data_fragment_unique_overlapping_t*>(&bounds));
            //no need to clear fragmentsNeedingResent, it will be cleared by GetBoundsMinusFragments
            FragmentSet::GetBoundsMinusFragments(bounds, m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver, fragmentsNeedingResent);
#endif
            // Send the data segments immediately if the out-of-order deferral feature is disabled (i.e. (time_duration == not_a_date_time))
            // which is needed for TestLtpEngine.
            if (m_ltpSessionSenderCommonDataRef.m_timeManagerOfSendingDelayedDataSegmentsRef.GetTimeDurationRef() == boost::posix_time::special_values::not_a_date_time) { //disabled
                ResendDataFromReport(fragmentsNeedingResent, reportSegment.reportSerialNumber); //will do nothing if fragmentsNeedingResent.empty() (i.e. this rs Has No Gaps In its Claims)
            }
            else {
                // Github issue 24: Defer data retransmission with out-of-order report segments
                //
                // When the network is causing out-of-order segment reception it is possible that one or more
                // synchronous reception reports are received either out-of-order or within a short time window,
                // possibly followed by an asynchronous reception report (see #23) indicating that the full red
                // part was received. To avoid unnecessary data retransmission the sending engine should defer
                // sending gap-filling data segments until some small window of time after the last reception
                // report for that session.
                //
                // Upon receiving a reception report the sending engine should (in addition to sending a report ack segment):
                //
                // 1.) Add (as a set union) the report's claims to the total claims seen for the session.
                // 2.) If a retransmission timer is not running:
                //      a.) If there are gaps in this report's claims (between its lower and upper bounds)
                //          (Note: gaps exist because the report itself has gaps AND no prior received report filled those gaps),
                //          then add this report to a pending report list and start a retransmission timer for the session
                // 3.) Otherwise, (i.e. If a retransmission timer is running):
                //      a.) add this report to the pending report list
                //      b.) if there are no gaps between the smallest lower bound of the pending report list
                //          and the largest upper bound of the pending report list then:
                //             stop the timer and clear the list (there is no need to retransmit
                //             data segments from any of the reports in the list)
                //
                // When the retransmission timer expires (i.e. there are still gaps to send) then send data segments to cover the remaining gaps for the session.
                // This involves iterating through the pending report list starting from the report with the lowest lowerBound
                // and transmitting the report serial number as the checkpoint for its last segment.
                // Subsequent reports in the iteration of the list should be ignored if everything
                // between their upper and lower bounds are fully contained in the set union
                // of ((total claims seen for the session) UNION (the just sent data segments of the prior reports in this list)).
                //
                // The result of this procedure is that the sending engine will not send either duplicate data segments to
                // cover gaps in earlier-sent reports which are claimed in later-sent reports. In the case of no loss but
                // highly out-of-order this will result in no unnecessary data retransmission to occur.

                if (m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration.empty()) { //timer is not running
                    if (!fragmentsNeedingResent.empty()) { //the only rs has gaps in the claims
                        //start the timer
                        m_largestEndIndexPendingGeneration = boundsUnique.endIndex;
                        m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration.emplace(boundsUnique, reportSegment.reportSerialNumber);
                        if (!m_ltpSessionSenderCommonDataRef.m_timeManagerOfSendingDelayedDataSegmentsRef.StartTimer(
                            this, M_SESSION_ID.sessionNumber, &m_ltpSessionSenderCommonDataRef.m_delayedDataSegmentsTimerExpiredCallbackRef))
                        {
                            LOG_ERROR(subprocess) << "LtpSessionSender::ReportSegmentReceivedCallback: unable to start m_timeManagerOfSendingDelayedDataSegmentsRef timer";
                        }
                    }
                    //else no work to do (no data segments to send)
                }
                else { //timer is running
                    m_largestEndIndexPendingGeneration = std::max(m_largestEndIndexPendingGeneration, boundsUnique.endIndex);
                    m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration.emplace(boundsUnique, reportSegment.reportSerialNumber);
                    const uint64_t largestBeginIndexPendingGeneration = m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration.begin()->first.beginIndex; //based on operator <
                    const bool pendingReportsHaveNoGapsInClaims = LtpFragmentSet::ContainsFragmentEntirely(m_ltpSessionSenderRecycledDataUniquePtr->m_dataFragmentsAckedByReceiver,
                        LtpFragmentSet::data_fragment_t(largestBeginIndexPendingGeneration, m_largestEndIndexPendingGeneration));
                    if (pendingReportsHaveNoGapsInClaims) {
                        m_ltpSessionSenderCommonDataRef.m_numDeletedFullyClaimedPendingReports += m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration.size();
                        //since there is a retransmission timer running stop it (there is no need to retransmit in this case)
                        //This overload of DeleteTimer auto-recycles userData (however userData not used in this timer so doesn't matter)
                        if (!m_ltpSessionSenderCommonDataRef.m_timeManagerOfSendingDelayedDataSegmentsRef.DeleteTimer(M_SESSION_ID.sessionNumber)) {
                            LOG_ERROR(subprocess) << "LtpSessionSender::ReportSegmentReceivedCallback: did not delete timer in m_timeManagerOfSendingDelayedDataSegmentsRef";
                        }
                        m_ltpSessionSenderRecycledDataUniquePtr->m_mapRsBoundsToRsnPendingGeneration.clear(); //also used as flag to signify timer no longer running
                    }
                    //else since there are gaps in the claims, add to the retransmission timer for the session (already done above through m_mapRsBoundsToRsnPendingGeneration.emplace)
                    
                }
            }
            
        }
    }
    if (!m_didNotifyForDeletion) {
        m_ltpSessionSenderCommonDataRef.m_notifyEngineThatThisSenderHasProducibleDataFunctionRef(M_SESSION_ID.sessionNumber);
    }
}

void LtpSessionSender::ResendDataFromReport(const LtpFragmentSet::data_fragment_set_t& fragmentsNeedingResent, const uint64_t reportSerialNumber) {
    for (LtpFragmentSet::data_fragment_set_t::const_iterator it = fragmentsNeedingResent.cbegin(); it != fragmentsNeedingResent.cend(); ++it) {
        const bool isLastFragmentNeedingResent = (boost::next(it) == fragmentsNeedingResent.cend());
        for (uint64_t dataIndex = it->beginIndex; dataIndex <= it->endIndex; ) {
            uint64_t bytesToSendRed = std::min((it->endIndex - dataIndex) + 1, m_ltpSessionSenderCommonDataRef.m_mtuClientServiceData);
            if ((bytesToSendRed + dataIndex) > M_LENGTH_OF_RED_PART) {
                LOG_FATAL(subprocess) << "gt length red part";
            }
            const bool isLastPacketNeedingResent = ((isLastFragmentNeedingResent) && ((dataIndex + bytesToSendRed) == (it->endIndex + 1)));
            const bool isEndOfRedPart = ((bytesToSendRed + dataIndex) == M_LENGTH_OF_RED_PART);
            if (isEndOfRedPart && !isLastPacketNeedingResent) {
                LOG_FATAL(subprocess) << "end of red part but not last packet being resent";
            }

            uint64_t checkpointSerialNumber = 0; //dont care
            LTP_DATA_SEGMENT_TYPE_FLAGS flags = LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA;
            if (isLastPacketNeedingResent) {
                flags = LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT;
                checkpointSerialNumber = m_nextCheckpointSerialNumber++; //now we care since this is now a checkpoint
                if (isEndOfRedPart) {
                    flags = LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART;
                    const bool isEndOfBlock = (M_LENGTH_OF_RED_PART == m_dataToSendSharedPtr->size());
                    if (isEndOfBlock) {
                        flags = LTP_DATA_SEGMENT_TYPE_FLAGS::REDDATA_CHECKPOINT_ENDOFREDPART_ENDOFBLOCK;
                    }
                }
            }

            m_ltpSessionSenderRecycledDataUniquePtr->m_resendFragmentsFlistQueue.emplace_back(dataIndex, bytesToSendRed, checkpointSerialNumber, reportSerialNumber, flags);
            dataIndex += bytesToSendRed;
        }
    }
}
