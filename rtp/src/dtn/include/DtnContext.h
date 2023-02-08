// Enables DTN compatibility with the uvgRTP context class
#pragma once



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

    std::shared_ptr<DtnSession> CreateDtnSession();

    size_t GetMTU();

};


