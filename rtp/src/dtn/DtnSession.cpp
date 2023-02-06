#include "DtnSession.h"

// : uvgrtp::session(cname, unused_addr), m_cname(cname) 
// DtnSession::DtnSession()
// {
//     session(cname, unused_addr);
// }

// DtnSession::~DtnSession()
// {

// }

std::shared_ptr<DtnMediaStreamSource> DtnSession::CreateDtnMediaStreamSource(rtp_format_t fmt, int rce_flags)
{
    // not currently supported
    if (rce_flags & RCE_RECEIVE_ONLY)
        return nullptr;

    if (rce_flags & RCE_OBSOLETE)
    {
        printf("You are using a flag that has either been removed or has been enabled by default. Consider updating RCE flags");
    }

    if ((rce_flags & RCE_SEND_ONLY) && (rce_flags & RCE_RECEIVE_ONLY))
    {
        printf("Cannot both use RCE_SEND_ONLY and RCE_RECEIVE_ONLY!");
        return nullptr;
    }

    return std::make_shared<DtnMediaStreamSource>(m_cname, fmt, rce_flags, 30, 1, 30); // todo make this configurable
}

rtp_error_t DtnSession::DestroyDtnMediaStreamSource(DtnMediaStreamSource *stream)
{
    if (!stream)
        return RTP_INVALID_VALUE;

    // not implemented

    return RTP_OK;
}