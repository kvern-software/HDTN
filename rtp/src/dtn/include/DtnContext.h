// Enables DTN compatibility with the uvgRTP context class
#pragma once



// DTN
#include "DtnSession.h"
#include "DtnUtil.h"

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
    DtnContext();
    ~DtnContext();


    std::shared_ptr<DtnSession> CreateDtnSession(rtp_modes_t operating_mode);

    size_t GetMTU();

};


