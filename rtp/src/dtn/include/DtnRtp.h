#pragma once

#include "DtnUtil.h"

#include <memory>
#include <vector>
#include <chrono>
#include <memory>
#include <atomic>
#include <arpa/inet.h>
/**
 * This class effectively acts as a tracker for all the
 * pertinent RTP frame information such as timestamp, ssrc,
 * number of packets sent, clock rate etc
*/
class DtnRtp
{
private:
    std::shared_ptr<std::atomic<uint32_t>> m_ssrc; // as seen in rtp frames
    uint32_t m_timestamp; // as seen in rtp frames
    uint16_t m_sequence; // as seen in rtp frames
    
    
    rtp_format_t m_fmt;
    uint32_t m_clockRate; // sampling clock rate, not hardware
    std::chrono::time_point<std::chrono::high_resolution_clock> m_wallClockStart; // filled upon first call to FillHeader

    size_t m_sentPackets; // number of sent packets

    size_t m_maxPayloadSize; // maximum payload of RTP packet, not bundle

    // size_t m_delay; // max delay before all fragments of a fragmented rtp packet are received before it is dropped


public:
    // each rtp packet must have a particular format and ssrc. get this information from the MediaStream object on creation
    DtnRtp(rtp_format_t fmt, std::shared_ptr<std::atomic<std::uint32_t>> ssrc);
    ~DtnRtp();

    // some helpful getters
    uint32_t     GetSsrc()          const;
    uint16_t     GetSequence()      const;
    uint32_t     GetClockRate()    const;
    size_t       GetPayloadSize()  const;
    size_t       GetPktMaxDelay() const;
    // rtp_format_t GetPayload()       const;

    // handles rtp paremeters 
    void IncSentPkts();
    void IncSequence();

    // setters for the rtp packet configuration
    void SetClockRate(rtp_format_t fmt); // this is not hardware clock. this is the sampling frequency of the given format. usually 90 kHz
    void SetDynamicPayload(uint8_t payload);
    void SetTimestamp(uint32_t timestamp); 
    void SetPayloadSize(size_t payload_size); // this should come from the media stream object
    void SetPktMaxDelay(size_t delay); // this should come from the media stream object

    void FillHeader(uint8_t *buffer);     // takes pointer to a rtp frame and fills the header with the current information about the rtp session
    void UpdateSequence(uint8_t *buffer); // takes pointer to a rtp frame and updates the header with the curent sequence number

};