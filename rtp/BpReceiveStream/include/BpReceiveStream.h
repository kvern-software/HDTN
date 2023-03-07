#pragma once

#include "app_patterns/BpSinkPattern.h"

#include "DtnRtp.h"

class BpReceiveStream : public BpSinkPattern {
public:
    BpReceiveStream(uint16_t outgoingRtpPort);
    virtual ~BpReceiveStream() override;

    std::shared_ptr<DtnRtp> m_outgoingDtnRtpPtr; // export our rtp frames using this object
    

protected:
    virtual bool ProcessPayload(const uint8_t * data, const uint64_t size) override;
private:


};
