// Enables DTN compatibility with the uvgRTP context class
#pragma once
// uvgRTP
#include <uvgrtp/context.hh>
#include <uvgrtp/lib.hh>

// DTN
#include "DtnSession.h"


class DtnSession;

/**
 * A DtnContext handles multiple DtnSessions.
*/
class DtnContext
{

private:
    /* Generate CNAME for participant using host and login names */
    std::string generate_cname() const;
    
    std::string get_cname();
    std::string m_cname;

    size_t m_MTU;
public:
    DtnContext(size_t maxTransmissionUnit) : m_MTU(maxTransmissionUnit) 
    {
        m_cname = get_cname();
    };

    ~DtnContext(){};

    /**
     * \brief Create a DTN RTP session
     * 
     * \param address IPv4 address of the remote participant           *
     * \return RTP session object
     *
     * \retval DtnSession      On success
    *  \retval nullptr          If "address" is empty or memory allocation failed
    */
    std::shared_ptr<DtnSession> CreateDtnSession();

    rtp_error_t DestroyDtnSession(std::shared_ptr<DtnSession> session);


    size_t GetMTU();

};


