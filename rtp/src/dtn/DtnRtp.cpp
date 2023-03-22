#include "DtnRtp.h"
#include "DtnUtil.h"
#include <random>
#include <cstring>
#include "Logger.h"


#define INVALID_TS UINT32_MAX
#define INVALID_SEQ (-1)

#define DEFAULT_MAX_PAYLOAD 1440


static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

//  The initial value of the timestamp SHOULD be random, as for the
//     sequence number.  Several consecutive RTP packets will have equal
//     timestamps if they are (logically) generated at once, e.g., belong
//     to the same video frame.  

    // Consecutive RTP packets MAY contain
    // timestamps that are not monotonic if the data is not transmitted
    // in the order it was sampled, as in the case of MPEG interpolated
    // video frames.  (The sequence numbers of the packets as transmitted
    // will still be monotonic.)




DtnRtp::DtnRtp(size_t maximumTransmissionUnit):
    m_clockRate(0),
    m_sentPackets(0),
    m_maximumTransmissionUnit(maximumTransmissionUnit)
{
    m_prevHeader.timestamp = INVALID_TS;
    m_prevHeader.seq = INVALID_SEQ;
}

DtnRtp::~DtnRtp()
{
}

// some helpful getters
uint32_t DtnRtp::GetSsrc()  const
{
    return *m_ssrc.get();
}


uint16_t DtnRtp::GetSequence()      const
{
    return m_prevHeader.seq;
}

uint32_t DtnRtp::GetTimestamp() const
{
    return m_prevHeader.timestamp;
}

uint32_t DtnRtp::GetClockRate()     const
{
    return m_clockRate;
}

rtp_header * DtnRtp::GetHeader() {
    return &m_prevHeader;
}

/**
 * This returns the number of times the current frame has been added to. 
 * A frame with no data is zero. A frame that has been added to once is 1. 
 * A frame that has been added two twice is 2. and so on.
*/
uint16_t DtnRtp::GetNumConcatenated()
{
    return m_numConcatenated;
}


// handles rtp paremeters
void DtnRtp::IncSentPkts()
{
    m_sentPackets++;
}
void DtnRtp::IncSequence()
{
    m_prevHeader.seq = htons(ntohs(m_prevHeader.seq) + 1) ;
}

void DtnRtp::IncNumConcatenated()
{
    m_numConcatenated++;
}

void DtnRtp::ResetNumConcatenated()
{
    m_numConcatenated = 0;
}

void DtnRtp::SetSequence(uint16_t host_sequence)
{
    m_prevHeader.seq = htons(host_sequence);
}

void DtnRtp::SetMarkerBit(uint8_t marker_bit)
{
    m_prevHeader.marker =  (m_prevHeader.marker | RTP_MARKER_FLAG);
}

// setters for the rtp packet configuration
void DtnRtp::SetClockRate(rtp_format_t fmt)
{
    switch (fmt) {
        case RTP_FORMAT_H264:
        case RTP_FORMAT_H265:
        case RTP_FORMAT_DYNAMICRTP:
            m_clockRate = 90000;
            break;
        default:
            printf("Unknown RTP format, setting clock rate to 8000");
            m_clockRate = 8000;
            break;
    }
}

void DtnRtp::SetTimestamp(uint32_t timestamp)
{
    m_prevHeader.timestamp = timestamp;
}

rtp_packet_status_t DtnRtp::PacketHandler(padded_vector_uint8_t &wholeBundleVec, const rtp_header * currentRtpFrameHeader)
{    
    if (wholeBundleVec.size() < 12) {       
        LOG_ERROR(subprocess) << "Received UDP packet is too small to contain RTP header, discarding...";
        return RTP_INVALID_HEADER;
    }
    // LOG_DEBUG(subprocess) << " In handler";

    rtp_frame * incomingFramePtr = (rtp_frame *) wholeBundleVec.data();
    rtp_header * incomingHeaderPtr = &incomingFramePtr->header;

    rtp_header_union_t currentHeaderFlags;
    memcpy(&currentHeaderFlags.flags, incomingHeaderPtr, sizeof(uint16_t));
    currentHeaderFlags.flags = htons(currentHeaderFlags.flags);

    // incomingFramePtr->print_header();

    if (!(RTP_VERSION_TWO_FLAG & currentHeaderFlags.flags)) {
        LOG_ERROR(subprocess) << "Unsupported RTP version. Use RTP Version 2";
        return RTP_INVALID_VERSION;
    }

    // This is indicative that we have received the first message, handle correspondingly
    if (!m_ssrc) {
        rtp_format_t fmt = (rtp_format_t) (RTP_PAYLOAD_MASK & currentHeaderFlags.flags); // use mask to find out our payload type

        LOG_INFO(subprocess) << "No active session. Creating active session with SSRC = " << incomingHeaderPtr->ssrc << "\n" \
                << "RTP Format: " << fmt << "\n" \
                << "Initial TS: " << ntohl(incomingHeaderPtr->timestamp) << "\n" \
                << "Initial Seq: " << ntohs(incomingHeaderPtr->seq);

        uint32_t ssrc = incomingHeaderPtr->ssrc; // extracting from packed struct
        m_ssrc = std::make_shared<std::atomic<uint32_t>>(ssrc); // assign ssrc 
        
        
        UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ); // set the previous header to match current header for the first packet
        SetClockRate(fmt); // assign our payload's clock rate 

        return RTP_FIRST_FRAME; // no need to check for all the things below, just start new frame
    }
    
    /**
     * Use the same SSRC for incoming and outgoing, assume we only have 1 BpSendStream per media source per CCSDS standard.
     * SSRC is assigned when the first UDP packet arrives to the UdpHandler. If prevHeader.ssrc is unassigned, it gets assigned. 
     * the incoming SSRC != assigned SSRC, then the packet is discarded. 
    */
//     if (incomingHeaderPtr->ssrc != GetSsrc()) { // CCSDS 3.3.7  & 3.3.8
//         LOG_ERROR(subprocess) << "Received mismatched SSRC! Original SSRC: " <<   GetSsrc() << " New SSRC: " << incomingHeaderPtr->ssrc;
//         LOG_ERROR(subprocess) << "Discarding new mismatched SSRC!";
//         return RTP_MISMATCH_SSRC;
//     }

//     if (ntohs(incomingHeaderPtr->seq) !=  (ntohs(m_prevHeader.seq)+1)) {
//         LOG_ERROR(subprocess) << "RTP sequence out of order - Incoming: " << ntohs(incomingHeaderPtr->seq) << " Previous: " << ntohs(m_prevHeader.seq);
//         UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ); // update prevHeader
//         return RTP_OUT_OF_SEQ;         // TODO : handle sequence out of order
//     }

//     // LOG_DEBUG(subprocess) << "Incoming sequence: " << ntohs(incomingHeaderPtr->seq);

//     // boost::this_thread::sleep_for(boost::chrono::milliseconds(250));

//     // This is where we start to apply the CCSDS rules 
//     rtp_header_union_t prevHeaderFlags;
//     memcpy(&prevHeaderFlags.flags, &m_prevHeader, sizeof(uint16_t));
//     prevHeaderFlags.flags = htons(prevHeaderFlags.flags);

//     int64_t deltat = ntohl(m_prevHeader.timestamp) - ntohl(incomingHeaderPtr->timestamp);
//     if (deltat != 0) { // CCSDS 3.3.4 // do not concatenate if the timestamp has changed
//         UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);
//         // LOG_INFO(subprocess) << "Timestamp changed! \nPrevious TS: " << ntohl(m_prevHeader.timestamp) << 
//         // " \nincoming TS: " << ntohl(incomingHeaderPtr->timestamp) << 
//         // " \nDelta: " << ntohl(m_prevHeader.timestamp)- ntohl(incomingHeaderPtr->timestamp);

//         return RTP_PUSH_PREVIOUS_FRAME;
//     }

//     if (RTP_PADDING_FLAG & currentHeaderFlags.flags) {  // CCSDS 3.3.2     // Do not concatenate if the padding bit is set
//         UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);
//         LOG_DEBUG(subprocess) << "Padding bit set";
//         return RTP_PUSH_PREVIOUS_FRAME;
//     }

//     if ((prevHeaderFlags.flags & RTP_MARKER_FLAG)  != (currentHeaderFlags.flags & RTP_MARKER_FLAG)) { // CCSDS 3.3.5 // Do not concatenate if the incoming marker bit has changed from the current packet's marker bit
//         UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);
//         // LOG_INFO(subprocess) << "marker changed";
//         return RTP_PUSH_PREVIOUS_FRAME;
//     }

//     if (currentRtpFrameHeader->ext != incomingHeaderPtr->ext) { // CCSDS 3.3.6 // Do not concatentate if the extension bit has changed
//         LOG_INFO(subprocess) << "ext changed";
//         UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);
//         return RTP_PUSH_PREVIOUS_FRAME;
//     }



    
//     /**
//      * If we have gotten to this point... 
//      *  1. We have a good RTP frame
//      *  2. This RTP frame qualifies to be concatentated 
//      * Proceed to concatenate the frame. 
//     */
//     // LOG_INFO(subprocess) << "Packet valid for concatentation";
//    UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);

    return RTP_PUSH_PREVIOUS_FRAME;
}

// rtp_packet_status_t DtnRtp::BundleHandler(padded_vector_uint8_t &wholeBundleVec)
// {
    
// }

/**
 * Take an incoming DtnSession header and update our own header accordingly.
 * We copy everything execept the sequence number, since our session will be on
 * our own sequence number
*/

void DtnRtp::UpdateHeader(const rtp_header * nextHeaderPointer, bool useIncomingSeq)
{
    // LOG_DEBUG(subprocess) << "updated header";
    if (useIncomingSeq) {
        memcpy(&m_prevHeader, nextHeaderPointer, sizeof(rtp_header)); // update previous header to current header
    } else {
        uint16_t currentSeq = m_prevHeader.seq;
        // uint32_t currentTs = m_prevHeader.timestamp;
        memcpy(&m_prevHeader, nextHeaderPointer, sizeof(rtp_header)); // update previous header to current header
        m_prevHeader.seq = currentSeq;
        
        // m_prevHeader.timestamp = currentTs;
    }
}