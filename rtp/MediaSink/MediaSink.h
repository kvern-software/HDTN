#pragma once

/**
 * This class inherits from BpSinkPatter. We hook up our DtnMediaStream to the source.
 * The source will unbundle and place the data into the media stream frame buffer 
*/

#include "app_patterns/BpSinkPattern.h"
#include "Logger.h"

#include "DtnMediaStream.h"
#include "DtnRtp.h"
 
struct FinalStatsBpSink {
    FinalStatsBpSink() : m_totalBytesRx(0), m_totalBundlesRx(0), m_receivedCount(0), m_duplicateCount(0),
        m_seqHval(0), m_seqBase(0) {};
    uint64_t m_totalBytesRx;
    uint64_t m_totalBundlesRx;
    uint64_t m_receivedCount;
    uint64_t m_duplicateCount;
    uint64_t m_seqHval;
    uint64_t m_seqBase;
};


class MediaSink : public  BpSinkPattern
{
private:

public:


    MediaSink(std::shared_ptr<DtnMediaStream> dtnMediaStream);
    ~MediaSink();

    std::shared_ptr<DtnMediaStream> m_DtnMediaStreamPtr;


    FinalStatsBpSink m_FinalStatsBpSink;
   
protected:
    virtual bool ProcessPayload(const uint8_t * data, const uint64_t size) override;
};


