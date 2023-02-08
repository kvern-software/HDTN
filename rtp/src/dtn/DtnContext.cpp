#include "DtnContext.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>

#define NAME_MAXLEN 512

std::string get_hostname()
{
#ifdef _WIN32
    char buffer[NAME_MAXLEN];
    DWORD bufCharCount = NAME_MAXLEN;

    if (!GetComputerName((TCHAR *)buffer, &bufCharCount))
        log_platform_error("GetComputerName() failed");

    return std::string(buffer);
#else
    char hostname[NAME_MAXLEN];

    if (gethostname(hostname, NAME_MAXLEN) != 0) {
        return "";
    }

    return std::string(hostname);
#endif
}

std::string get_username()
{
#ifdef _WIN32
    char buffer[NAME_MAXLEN];
    DWORD bufCharCount = NAME_MAXLEN;

    if (!GetUserName((TCHAR *)buffer, &bufCharCount)) {
        log_platform_error("GetUserName() failed");
        return "";
    }

    return std::string(buffer);
#else
    char username[NAME_MAXLEN];

    if (getlogin_r(username, NAME_MAXLEN) != 0) {
        return "";
    }

    return std::string(username);
#endif
}


static inline std::string generate_string(size_t length)
{
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };

    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}


std::shared_ptr<DtnSession> DtnContext::CreateDtnSession() 
{
    return std::make_shared<DtnSession>(m_cname);
}


std::string DtnContext::generate_cname() const
{
    std::string host = get_hostname();
    std::string user = get_username();

    if (host == "")
        host = generate_string(10);

    if (user == "")
        user = generate_string(10);

    return host + "@" + user;
}

std::string DtnContext::get_cname()
{
    return m_cname;
}

size_t DtnContext::GetMTU()
{
    return m_MTU;
}