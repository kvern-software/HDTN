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

// void DtnRtp::SetDynamicPayload(uint8_t payload);
void DtnRtp::SetTimestamp(uint32_t timestamp)
{
    m_prevHeader.timestamp = timestamp;
}

void DtnRtp::FillHeader(rtp_frame * frame)
{
    // if (!frame)
    //     return;

    // /* This is the first RTP message, get wall clock reading (t = 0)
    //  * and generate random RTP timestamp for this reading */
    // if (m_timestamp == INVALID_TS) {
    //     m_timestamp        = GenRandom();
    //     m_wallClockStart = std::chrono::high_resolution_clock::now();
    // }

    // frame->header.version = 2;
    // frame->header.padding = 0;
    // frame->header.ext = 0;
    // frame->header.cc = 0; // not implemented
    // frame->header.marker = 0;
    // frame->header.payload = (m_fmt & 0x7f) | (0 << 7);
    // // frame->header.seq =  htons(m_sequence);


    // auto now = std::chrono::high_resolution_clock::now();
    // std::chrono::microseconds wall_time_since_start = 
    //     std::chrono::duration_cast<std::chrono::microseconds>(now - m_wallClockStart); // wall time total

    // uint64_t u_seconds_elapsed = wall_time_since_start.count() * m_clockRate; // time elapsed using our media clock rate
    // uint32_t rtp_timestamp = m_timestamp + uint32_t(u_seconds_elapsed / 1000000); // convert from micro seconds to seconds


    // frame->header.timestamp = htonl((u_long)rtp_timestamp);
    // frame->header.ssrc = htonl(*m_ssrc.get());
}

void DtnRtp::UpdateSequence(rtp_frame * frame)
{
    // if (!frame)
    //     return;

    // frame->header.seq = htons(m_sequence); //network byte order

    // IncSequence();
}


/**
 * Parses payload into rtp packets
*/
int DtnRtp::PacketHandler(ssize_t size, void *packet, int rce_flags,  std::shared_ptr<DtnFrameQueue> incomingFrameQueue)
{
//     (void)rce_flags;
//     /* not an RTP frame */
//     if (size < 12) {       
//         LOG_ERROR(subprocess) << "Received RTP packet is too small to contain header";
//         return -1;
//     }
    
//     rtp_frame tmp_frame; // allocate new frame to be filled
//     rtp_frame * incomingFramePtr = (rtp_frame *) packet;
//     // uint8_t *ptr = (uint8_t *)packet;

//     /* invalid version */
//     if (incomingFramePtr->header.version != 2) {
//         LOG_ERROR(subprocess) << "Received RTP packet with invalid version";
//         return -1;
//     }

//     tmp_frame.header.version   = (unsigned int) incomingFramePtr->header.version;
//     tmp_frame.header.padding   = (unsigned int) incomingFramePtr->header.padding;
//     tmp_frame.header.ext       = (unsigned int) incomingFramePtr->header.ext; 
//     tmp_frame.header.cc        = (unsigned int) incomingFramePtr->header.cc; 
//     tmp_frame.header.marker    = (unsigned int) incomingFramePtr->header.marker;
//     tmp_frame.header.payload   = (unsigned int) incomingFramePtr->header.payload; 
//     tmp_frame.header.seq       =  (incomingFramePtr->header.seq);     // still in network byte order here
//     tmp_frame.header.timestamp =  (incomingFramePtr->header.timestamp); // still in network byte order here
//     tmp_frame.header.ssrc      =  (incomingFramePtr->header.ssrc);

//     tmp_frame.payload.length = (size_t)size - sizeof(rtp_header);
//     tmp_frame.print_header();

//     // /* Skip the generics RTP header
//     //  * There may be 0..N CSRC entries after the header, so check those */
//     // ptr += sizeof(rtp_header);

//     if (tmp_frame.header.cc > 0) {
//        // LOG_INFO(subprocess) << "frame contains csrc entries";

//  //       if ((ssize_t) (tmp_frame.payload.length- tmp_frame.header.cc * sizeof(uint32_t) ) < 0) {
//    //         LOG_ERROR(subprocess) << "Invalid frame length, " << tmp_frame.header.cc << "CSRC entries, total length " << tmp_frame.payload.length;
//      //       return -1;
//        // }

//        // LOG_INFO(subprocess) << "Allocating"  << tmp_frame.header.cc << "CSRC entries";

//         // (*out)->csrc         = new uint32_t[(*out)->header.cc];
//         // (*out)->payload_len -= (*out)->header.cc * sizeof(uint32_t);

//         // for (size_t i = 0; i < (*out)->header.cc; ++i) {
//         //     (*out)->csrc[i]  = *(uint32_t *)ptr;
//         //     ptr             += sizeof(uint32_t);
//         // }
//     }

//     if (tmp_frame.header.ext) {
//         LOG_INFO(subprocess) << "Frame contains extension information";
//         // (*out)->ext = new uvgrtp::frame::ext_header;
//         // (*out)->ext->type    = ntohs(*(uint16_t *)&ptr[0]);
//         // (*out)->ext->len     = ntohs(*(uint16_t *)&ptr[2]) * sizeof(uint32_t);
//         // (*out)->ext->data    = (uint8_t *)memdup(ptr + 2 * sizeof(uint16_t), (*out)->ext->len);
//         // (*out)->payload_len -= 2 * sizeof(uint16_t) + (*out)->ext->len;
//         // ptr                 += 2 * sizeof(uint16_t) + (*out)->ext->len;
//     }

//     /* If padding is set to 1, the last byte of the payload indicates
//      * how many padding bytes was used. Make sure the padding length is
//      * valid and subtract the amount of padding bytes from payload length */
//     if (tmp_frame.header.padding) {
//         LOG_INFO(subprocess) << "Frame contains padding";
//         // uint8_t padding_len = (*out)->payload[(*out)->payload_len - 1];

//         // if (!padding_len || (*out)->payload_len <= padding_len) {
//         //     uvgrtp::frame::dealloc_frame(*out);
//         //     return -1;
//         // }

//         // (*out)->payload_len -= padding_len;
//         // (*out)->padding_len  = padding_len;
//     }

//     // copy out payload to tmp frame
//     // tmp_frame.payload.start = ptr;
//     tmp_frame.payload.allocate(tmp_frame.payload.length);
//     tmp_frame.payload.copy(packet + sizeof(rtp_header)); // todo correct this 

//     incomingFrameQueue->PushFrame(tmp_frame);
//     // tmp_frame.payload.start    =  //(uint8_t *) //memdup(ptr, (*out)->payload_len);
//     // tmp_frame.dgram      = (uint8_t *)packet;
    // tmp_frame.dgram_size = size;

    return 0;
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

    incomingFramePtr->print_header();

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
    if (incomingHeaderPtr->ssrc != GetSsrc()) { // CCSDS 3.3.7  & 3.3.8
        LOG_ERROR(subprocess) << "Received mismatched SSRC! Original SSRC: " <<   GetSsrc() << " New SSRC: " << incomingHeaderPtr->ssrc;
        LOG_ERROR(subprocess) << "Discarding new mismatched SSRC!";
        return RTP_MISMATCH_SSRC;
    }

    if (ntohs(incomingHeaderPtr->seq) !=  (ntohs(m_prevHeader.seq)+1)) {
        LOG_ERROR(subprocess) << "RTP sequence out of order - Incoming: " << ntohs(incomingHeaderPtr->seq) << " Previous: " << ntohs(m_prevHeader.seq);
        UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ); // update prevHeader
        return RTP_OUT_OF_SEQ;         // TODO : handle sequence out of order
    }

    // LOG_DEBUG(subprocess) << "Incoming sequence: " << ntohs(incomingHeaderPtr->seq);

    // boost::this_thread::sleep_for(boost::chrono::milliseconds(250));

    // This is where we start to apply the CCSDS rules 
    rtp_header_union_t prevHeaderFlags;
    memcpy(&prevHeaderFlags.flags, &m_prevHeader, sizeof(uint16_t));
    prevHeaderFlags.flags = htons(prevHeaderFlags.flags);

    // int64_t deltat = ntohl(m_prevHeader.timestamp)- ntohl(incomingHeaderPtr->timestamp);
    // std::cout << deltat << std::endl;
    // if (deltat != 0) { // CCSDS 3.3.4 // do not concatenate if the timestamp has changed
    //     UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);
    //     LOG_INFO(subprocess) << "Timestamp changed! \nPrevious TS: " << ntohl(m_prevHeader.timestamp) << \
    //     " \nincoming TS: " << ntohl(incomingHeaderPtr->timestamp) << \
    //     " \nDelta: " << ntohl(m_prevHeader.timestamp)- ntohl(incomingHeaderPtr->timestamp);

    //     return RTP_PUSH_PREVIOUS_FRAME;
    // }

    if (RTP_PADDING_FLAG & currentHeaderFlags.flags) {  // CCSDS 3.3.2     // Do not concatenate if the padding bit is set
        UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);
        LOG_DEBUG(subprocess) << "Padding bit set";
        return RTP_PUSH_PREVIOUS_FRAME;
    }

    if ((prevHeaderFlags.flags & RTP_MARKER_FLAG)  != (currentHeaderFlags.flags & RTP_MARKER_FLAG)) { // CCSDS 3.3.5 // Do not concatenate if the incoming marker bit has changed from the current packet's marker bit
        UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);
        LOG_INFO(subprocess) << "marker changed";
        return RTP_PUSH_PREVIOUS_FRAME;

    }

    if (currentRtpFrameHeader->ext != incomingHeaderPtr->ext) { // CCSDS 3.3.6 // Do not concatentate if the extension bit has changed
        LOG_INFO(subprocess) << "ext changed";
        UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);
        return RTP_PUSH_PREVIOUS_FRAME;
    }



    
    /**
     * If we have gotten to this point... 
     *  1. We have a good RTP frame
     *  2. This RTP frame qualifies to be concatentated 
     * Proceed to concatenate the frame. 
    */
    // LOG_INFO(subprocess) << "Packet valid for concatentation";
   UpdateHeader(incomingHeaderPtr, USE_INCOMING_SEQ);

    return RTP_CONCATENATE;
}


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

/**
 * Take a video frame and encapsulate into an RTP packet.
 * Follow the DTN/BP contatenation rules provided by CCSDSS
 * 
 *  - If SRTP (secure) is used, do not concatenate
 *  - All concatenated packets must have the same RTP timestamp. Effectively, this means only concatenate frames from the same frame.
 *      
*/ 
