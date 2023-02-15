#include "DtnSession.h"

// : uvgrtp::session(cname, unused_addr), m_cname(cname) 
// DtnSession::DtnSession()
// {
//     session(cname, unused_addr);
// }

// DtnSession::~DtnSession()
// {

// }

DtnSession::DtnSession(std::string cname, rtp_modes_t operating_mode) :
    m_cname(cname),
    m_operating_mode(operating_mode)
{

}

DtnSession::~DtnSession()
{

}


std::shared_ptr<DtnMediaStream> DtnSession::CreateDtnMediaStream(rtp_format_t fmt)
{
    // // not currently supported
    // if (rce_flags & RCE_RECEIVE_ONLY)
    //     return nullptr;

    // if (rce_flags & RCE_OBSOLETE)
    // {
    //     printf("You are using a flag that has either been removed or has been enabled by default. Consider updating RCE flags");
    // }

    // if ((rce_flags & RCE_SEND_ONLY) && (rce_flags & RCE_RECEIVE_ONLY))
    // {
    //     printf("Cannot both use RCE_SEND_ONLY and RCE_RECEIVE_ONLY!");
    //     return nullptr;
    // }


    return std::make_shared<DtnMediaStream>(m_cname, m_operating_mode); // todo make this configurable
}
