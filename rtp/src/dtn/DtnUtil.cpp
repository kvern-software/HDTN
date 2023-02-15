#include "DtnUtil.h"

uint32_t GenRandom()
{
    std::default_random_engine generator;
    std::uniform_int_distribution<uint32_t> distribution(0,  UINT32_MAX);
    return (uint32_t) distribution(generator);  
}
