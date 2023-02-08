#include "DtnRtp.h"
#include <random>

#define INVALID_TS UINT32_MAX
#define DEFAULT_MAX_PAYLOAD 1440

static uint32_t GenRandom()
{
    std::default_random_engine generator;
    std::uniform_int_distribution<uint32_t> distribution(0,  UINT32_MAX);
    return (uint32_t) distribution(generator);  
}

DtnRtp::DtnRtp(rtp_format_t fmt, std::shared_ptr<std::atomic<std::uint32_t>> ssrc):
    m_ssrc(ssrc),
    m_timestamp(INVALID_TS),
    m_sequence(GenRandom()),
    m_fmt(fmt),
    m_clockRate(0),
    m_sentPackets(0),
    m_maxPayloadSize(DEFAULT_MAX_PAYLOAD)
{
    SetClockRate(fmt);
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
    return m_sequence;
}


uint32_t DtnRtp::GetClockRate()     const
{
    return m_clockRate;
}

size_t DtnRtp::GetPayloadSize()   const
{
    return m_maxPayloadSize;
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
    if (m_sequence != UINT16_MAX) {
        m_sequence++;
    } else {
        m_sequence = 0;
    }
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
    m_timestamp = timestamp;
}

void DtnRtp::SetPayloadSize(size_t payload_size)
{
    m_maxPayloadSize = payload_size;
}

// void DtnRtp::SetPktMaxDelay(size_t delay)
// {
     // not implemented
// }

void DtnRtp::FillHeader(uint8_t *buffer)
{
    if (!buffer)
        return;

    /* This is the first RTP message, get wall clock reading (t = 0)
     * and generate random RTP timestamp for this reading */
    if (m_timestamp == INVALID_TS) {
        m_timestamp        = GenRandom();
        m_wallClockStart = std::chrono::high_resolution_clock::now();
    }

    buffer[0] = 2 << 6; // RTP version
    buffer[1] = (m_fmt & 0x7f) | (0 << 7);

    *(uint16_t *)&buffer[2] = htons(m_sequence);
    *(uint32_t *)&buffer[8] = htonl(*m_ssrc.get());

    // todo, get timestamp figured out

    // if (timestamp_ == INVALID_TS) {

    //     auto t1 = std::chrono::high_resolution_clock::now();
    //     std::chrono::microseconds time_since_start = 
    //         std::chrono::duration_cast<std::chrono::microseconds>(t1 - wc_start_);

    //     uint64_t u_seconds = time_since_start.count() * clock_rate_;

    //     uint32_t rtp_timestamp = ts_ + uint32_t(u_seconds / 1000000);

    //     *(uint32_t *)&buffer[4] = htonl((u_long)rtp_timestamp);

    // } else {
    //     *(uint32_t *)&buffer[4] = htonl((u_long)timestamp_);
    // }
}

void DtnRtp::UpdateSequence(uint8_t *buffer)
{
    if (!buffer)
        return;

    *(uint16_t *)&buffer[2] = htons(m_sequence); //network byte order
}