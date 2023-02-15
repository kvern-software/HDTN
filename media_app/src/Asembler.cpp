#include "Assembler.h"

Assembler::Assembler(const ExportFrameCallback_t & exportFrameCallback)
{
    m_exportFrameCallback = exportFrameCallback;
}

Assembler::~Assembler()
{
}

Assembler::Start() 
{
    if (!m_running) {
        m_running = true;
        // std::cout << "starting " << std::endl;
        m_AssemblerThreadPtr = boost::make_unique<boost::thread>(
        boost::bind(&Assembler::FragmentAssembler, this)); //create and start the worker thread
    }
}

Assembler::Stop() 
{
    m_running = false; //thread stopping criteria

    if(m_AssemblerThreadPtr) {
        m_AssemblerThreadPtr->join();
        m_AssemblerThreadPtr.reset(); //delete it
    }
}

Assembler::FragmentAssembler() 
{

    while (1) 
    {
        // iterate through queue of fragments
        for (auto itr = m_fragmentQueue.front(); itr != m_fragmentQueue.back(); itr++)
        {
            // if we received a fragment with a frameID greater than our current frame ID, drop current frame for the next frame

            if (itr.header.fragment_id == m_currentFragmentID) 
            {
                // push fragment into frame buffer

            } 
            else if (iter.header.fragment_id > m_currentFragmentID)
            {
                // do nothing
            } 
            else if (iter.header.fragment_id < m_currentFragmentID) 
            {
                // do nothing
            }

        }
    }
}