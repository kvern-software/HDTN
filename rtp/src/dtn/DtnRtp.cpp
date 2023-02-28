#include "DtnRtp.h"
#include "RtpFrame.h"
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


uint32_t DtnRtp::GetClockRate()     const
{
    return m_clockRate;
}

size_t DtnRtp::GetPayloadSize()   const
{
    // return m_rtpMTU - sizeof(rtp_header);
}

size_t DtnRtp::GetPktMaxDelay()   const
{
    return 0; // not implemented
}


// handles rtp paremeters 
void DtnRtp::IncSentPkts()
{
    m_sentPackets++;
}
void DtnRtp::IncSequence()
{
    // if (m_sequence != UINT16_MAX) {
    //     m_sequence++;
    // } else {
    //     m_sequence = 0;
    // }
}

void DtnRtp::SetSequence(uint16_t host_sequence)
{
    m_prevHeader.seq = htons(host_sequence);
}

void DtnRtp::SetMarkerBit(uint8_t marker_bit)
{
    m_prevHeader.marker = marker_bit;
}

void DtnRtp::SetFormat(rtp_format_t fmt)
{
    m_fmt = fmt;
}

// setters for the rtp packet configuration
void DtnRtp::SetClockRate(rtp_format_t fmt)
{
    switch (fmt) {
        case RTP_FORMAT_H264:
        case RTP_FORMAT_H265:
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
    (void)rce_flags;
    /* not an RTP frame */
    if (size < 12) {       
        LOG_ERROR(subprocess) << "Received RTP packet is too small to contain header";
        return -1;
    }
    
    rtp_frame tmp_frame; // allocate new frame to be filled
    rtp_frame * frame_ptr = (rtp_frame *) packet;
    // uint8_t *ptr = (uint8_t *)packet;

    /* invalid version */
    if (frame_ptr->header.version != 2) {
        LOG_ERROR(subprocess) << "Received RTP packet with invalid version";
        return -1;
    }

    tmp_frame.header.version   = (unsigned int) frame_ptr->header.version;
    tmp_frame.header.padding   = (unsigned int) frame_ptr->header.padding;
    tmp_frame.header.ext       = (unsigned int) frame_ptr->header.ext; 
    tmp_frame.header.cc        = (unsigned int) frame_ptr->header.cc; 
    tmp_frame.header.marker    = (unsigned int) frame_ptr->header.marker;
    tmp_frame.header.payload   = (unsigned int) frame_ptr->header.payload; 
    tmp_frame.header.seq       =  (frame_ptr->header.seq);     // still in network byte order here
    tmp_frame.header.timestamp =  (frame_ptr->header.timestamp); // still in network byte order here
    tmp_frame.header.ssrc      =  (frame_ptr->header.ssrc);

    tmp_frame.payload.length = (size_t)size - sizeof(rtp_header);
    tmp_frame.print_header();

    // /* Skip the generics RTP header
    //  * There may be 0..N CSRC entries after the header, so check those */
    // ptr += sizeof(rtp_header);

    if (tmp_frame.header.cc > 0) {
       // LOG_INFO(subprocess) << "frame contains csrc entries";

 //       if ((ssize_t) (tmp_frame.payload.length- tmp_frame.header.cc * sizeof(uint32_t) ) < 0) {
   //         LOG_ERROR(subprocess) << "Invalid frame length, " << tmp_frame.header.cc << "CSRC entries, total length " << tmp_frame.payload.length;
     //       return -1;
       // }

       // LOG_INFO(subprocess) << "Allocating"  << tmp_frame.header.cc << "CSRC entries";

        // (*out)->csrc         = new uint32_t[(*out)->header.cc];
        // (*out)->payload_len -= (*out)->header.cc * sizeof(uint32_t);

        // for (size_t i = 0; i < (*out)->header.cc; ++i) {
        //     (*out)->csrc[i]  = *(uint32_t *)ptr;
        //     ptr             += sizeof(uint32_t);
        // }
    }

    if (tmp_frame.header.ext) {
        LOG_INFO(subprocess) << "Frame contains extension information";
        // (*out)->ext = new uvgrtp::frame::ext_header;
        // (*out)->ext->type    = ntohs(*(uint16_t *)&ptr[0]);
        // (*out)->ext->len     = ntohs(*(uint16_t *)&ptr[2]) * sizeof(uint32_t);
        // (*out)->ext->data    = (uint8_t *)memdup(ptr + 2 * sizeof(uint16_t), (*out)->ext->len);
        // (*out)->payload_len -= 2 * sizeof(uint16_t) + (*out)->ext->len;
        // ptr                 += 2 * sizeof(uint16_t) + (*out)->ext->len;
    }

    /* If padding is set to 1, the last byte of the payload indicates
     * how many padding bytes was used. Make sure the padding length is
     * valid and subtract the amount of padding bytes from payload length */
    if (tmp_frame.header.padding) {
        LOG_INFO(subprocess) << "Frame contains padding";
        // uint8_t padding_len = (*out)->payload[(*out)->payload_len - 1];

        // if (!padding_len || (*out)->payload_len <= padding_len) {
        //     uvgrtp::frame::dealloc_frame(*out);
        //     return -1;
        // }

        // (*out)->payload_len -= padding_len;
        // (*out)->padding_len  = padding_len;
    }

    // copy out payload to tmp frame
    // tmp_frame.payload.start = ptr;
    tmp_frame.payload.allocate(tmp_frame.payload.length);
    tmp_frame.payload.copy(packet + sizeof(rtp_header)); // todo correct this 

    incomingFrameQueue->PushFrame(tmp_frame);
    // tmp_frame.payload.start    =  //(uint8_t *) //memdup(ptr, (*out)->payload_len);
    // tmp_frame.dgram      = (uint8_t *)packet;
    // tmp_frame.dgram_size = size;

    return 0;
}


rtp_error_codes_t DtnRtp::PacketHandler(padded_vector_uint8_t &wholeBundleVec)
{
    static signed int previous_seq = -1;
    
    if (wholeBundleVec.size() < 12) {       
        LOG_ERROR(subprocess) << "Received UDP packet is too small to contain RTP header, discarding...";
        return RTP_INVALID_HEADER;
    }

    rtp_frame * frame_ptr = (rtp_frame *) wholeBundleVec.data();

    if (frame_ptr->header.version < 1) {
        LOG_ERROR(subprocess) << "Unsupported RTP version. Use RTP Version > 1";
        return RTP_INVALID_VERSION;
    }

    // This is indicitive that we have received the first message, handle correspondingly
    if ((previous_seq == INVALID_SEQ)) {
        SetSequence(ntohs(frame_ptr->header.seq)); // assign initial sequence number
        m_ssrc = std::make_shared<std::atomic<uint32_t>>(frame_ptr->header.ssrc); // assign ssrc 
        SetFormat((rtp_format_t ) frame_ptr->header.payload); // assign our payload type
        SetClockRate((rtp_format_t ) frame_ptr->header.payload); // assign our payloads' clock rate
        SetTimestamp(ntohl(frame_ptr->header.timestamp)); // assign initial timestamp
        SetMarkerBit((bool) frame_ptr->header.marker);
    }
    

    /**
     * Use the same SSRC for incoming and going, assume we only have 1 BpSendStream per media source per CCSDS standard.
     * SSRC is assigned when the first UDP packet arrives to the UdpHandler. If prevHeader.ssrc is unassigned, it gets assigned. 
     * the incoming SSRC != assigned SSRC, then the packet is discarded. 
    */
    if (frame_ptr->header.ssrc != GetSsrc()) { // CCSDS 3.3.7  & 3.3.8
        LOG_ERROR(subprocess) << "Received mismatched SSRC! Original SSRC: " <<   GetSsrc() << " New SSRC: " << frame_ptr->header.ssrc;
        LOG_ERROR(subprocess) << "Discarding new mismatched SSRC!";
        return RTP_MISMATCH_SSRC;
    }

    if (ntohs(frame_ptr->header.seq) != (ntohs(m_prevHeader.seq) + 1)) {
        std::cout << "sequence out of order" << ntohs(frame_ptr->header.seq) << std::endl;
        // TODO : handle sequence out of order
        // return RTP_OUT_OF_SEQ;
    }

    memcpy(&m_prevHeader, frame_ptr, sizeof(rtp_header)); // update previous header to current header

    // This is where we start to apply the CCSDS rules 
    
    // Do not concatenate if the padding bit is set
    if (frame_ptr->header.padding) {  // CCSDS 3.3.2
        // Notify that we need to send any packets we have in queue

        // add this packet to a new queue
    }

    if (m_prevHeader.marker != frame_ptr->header.marker) { // CCSDS 3.3.5
        // Notify that we need to send any packets we have in queue

        // add this packet to a new queue

    }

    if (m_prevHeader.ext != frame_ptr->header.ext) { // CCSDS 3.3.6
        // Notify that we need to send any packets we have in queue

        // add this packet to a new queue
    }


    if (m_prevHeader.timestamp != ntohl(frame_ptr->header.timestamp)) { // CCSDS 3.3.4
        // Notify that we need to send any packets we have in queue

        // add this packet to a new queue

    }

    
    /**
     * If we have gotten to this point... 
     *  1. We have a good RTP frame
     *  2. This RTP frame qualifies to be concatentated 
     * Proceed to concatenate the frame. 
    */




   

    return RTP_OK;
}

/**
 * Take a video frame and encapsulate into an RTP packet.
 * Follow the DTN/BP contatenation rules provided by CCSDSS
 * 
 *  - If SRTP (secure) is used, do not concatenate
 *  - All concatenated packets must have the same RTP timestamp. Effectively, this means only concatenate frames from the same frame.
 *      
*/ 
