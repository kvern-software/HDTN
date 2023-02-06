// enables DTN functionality for uvgRTP sessions

#pragma once
#include <uvgrtp/session.hh>

#include "DtnMediaStream.h"

class DtnMediaStreamSource;

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

public:
    DtnSession(std::string cname) : m_cname(cname){}


    std::shared_ptr<DtnMediaStreamSource> CreateDtnMediaStreamSource(rtp_format_t fmt, int rce_flags);
    rtp_error_t DestroyDtnMediaStreamSource(DtnMediaStreamSource * stream);

};


