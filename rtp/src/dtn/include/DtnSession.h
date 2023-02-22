// enables DTN functionality for uvgRTP sessions

#pragma once

#include "DtnMediaStream.h"
#include "DtnUtil.h"

class DtnMediaStream;

/**
 * A DtnSession handles multiple DtnMediaStreams. 
 * A DtnSession does nothing on its own. It creates DtnMediaStream(s)
 * which do the actual RTP streaming. A DtnMediaStream corresponds with an 
 * RTP session defined in RFC3550.
*/
class DtnSession
{
private:

    std::string m_cname;
    std::vector<uint32_t> m_ssrcList;
    rtp_modes_t m_operating_mode;

public:
    DtnSession(std::string cname, rtp_modes_t operating_mode);
    ~DtnSession();

    std::shared_ptr<DtnMediaStream> CreateDtnMediaStream(rtp_format_t fmt);
};


