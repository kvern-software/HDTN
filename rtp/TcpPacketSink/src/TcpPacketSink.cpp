#include <boost/bind/bind.hpp>
#include <memory>
#include "Logger.h"
#include "TcpPacketSink.h"
#include <boost/endian/conversion.hpp>
#include <boost/make_unique.hpp>
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

TcpPacketSink::TcpPacketSink(std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocketPtr,
    boost::asio::io_service & tcpSocketIoServiceRef,
    const WholePacketReadyCallback_t & wholePacketReadyCallback,
    const unsigned int numCircularBufferVectors,
    const uint32_t maxTcpPacketSize,
    const NotifyReadyToDeleteCallback_t & notifyReadyToDeleteCallback) :

    m_wholePacketReadyCallback(wholePacketReadyCallback),
    m_notifyReadyToDeleteCallback(notifyReadyToDeleteCallback),
    m_tcpSocketPtr(tcpSocketPtr),
    m_tcpSocketIoServiceRef(tcpSocketIoServiceRef),
    m_numCircularBufferElements(numCircularBufferVectors),
    m_maxTcpPacketSize(maxTcpPacketSize),
    m_tcpReceiveBuffer(m_maxTcpPacketSize),
    m_circularIndexBuffer(m_numCircularBufferElements),
    m_tcpReceiveBuffersCbVec(m_numCircularBufferElements),
    m_tcpReceiveBytesTransferredCbVec(m_numCircularBufferElements),
    m_stateTcpReadActive(false),
    m_printedCbTooSmallNotice(false),
    m_running(false),
    m_safeToDelete(false)
{
    m_telemetry.m_connectionName = tcpSocketPtr->remote_endpoint().address().to_string() 
        + ":" + boost::lexical_cast<std::string>(tcpSocketPtr->remote_endpoint().port());
    m_telemetry.m_inputName = std::string("*:") + boost::lexical_cast<std::string>(tcpSocketPtr->local_endpoint().port());
    LOG_INFO(subprocess) << "stcp sink using CB size: " << m_numCircularBufferElements;
    
    
    for (unsigned int i = 0; i < m_numCircularBufferElements; ++i) {
        m_tcpReceiveBuffersCbVec[i].resize(m_maxTcpPacketSize);
    }

    m_running = true;
    m_threadCbReaderPtr = boost::make_unique<boost::thread>(
        boost::bind(&TcpPacketSink::PopCbThreadFunc, this)); //create and start the worker thread

    TryStartTcpReceive();
}

TcpPacketSink::~TcpPacketSink() {

    if (!m_safeToDelete) {
        DoTcpShutdown();
        while (!m_safeToDelete) {
            try {
                boost::this_thread::sleep(boost::posix_time::milliseconds(250));
            }
            catch (const boost::thread_resource_error&) {}
            catch (const boost::thread_interrupted&) {}
            catch (const boost::condition_error&) {}
            catch (const boost::lock_error&) {}
        }
    }
    
    m_mutexCb.lock();
    m_running = false; //thread stopping criteria
    m_mutexCb.unlock();
    m_conditionVariableCb.notify_one();

    if (m_threadCbReaderPtr) {
        try {
            m_threadCbReaderPtr->join();
            m_threadCbReaderPtr.reset(); //delete it
        }
        catch (const boost::thread_resource_error&) {
            LOG_ERROR(subprocess) << "error stopping TcpPacketSink threadCbReader";
        }
    }
}

//Note: the tcp layer will control flow in the event that the source is faster than the sink
void TcpPacketSink::TryStartTcpReceive() {
    if ((!m_stateTcpReadActive) && (m_tcpSocketPtr)) {

        const unsigned int writeIndex = m_circularIndexBuffer.GetIndexForWrite(); //store the volatile
        if (writeIndex == CIRCULAR_INDEX_BUFFER_FULL) {
            if (!m_printedCbTooSmallNotice) {
                m_printedCbTooSmallNotice = true;
                LOG_WARNING(subprocess) << "TcpPacketSink::TryStartTcpReceive(): buffers full.. you might want to increase the circular buffer size!"; 
            }
        }
        else {
            //StartTcpReceiveIncomingBundleSize
            m_stateTcpReadActive = true;
            boost::asio::async_read(*m_tcpSocketPtr,
                boost::asio::buffer(m_tcpReceiveBuffer),
                boost::bind(&TcpPacketSink::HandleTcpReceiveIncomingBundleSize, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    writeIndex));
            
        }
    }
}


void TcpPacketSink::HandleTcpReceiveIncomingBundleSize(const boost::system::error_code & error, std::size_t bytesTransferred, const unsigned int writeIndex) {
    if (!error) {
        m_telemetry.m_totalStcpBytesReceived += bytesTransferred;
        // if (m_incomingBundleSize == 0) { //keepalive (0 is endian agnostic)
        //     LOG_INFO(subprocess) << "keepalive packet received";
        //     //StartTcpReceiveIncomingBundleSize
        //     boost::asio::async_read(*m_tcpSocketPtr,
        //         boost::asio::buffer(&m_incomingBundleSize, sizeof(m_incomingBundleSize)),
        //         boost::bind(&TcpPacketSink::HandleTcpReceiveIncomingBundleSize, this,
        //             boost::asio::placeholders::error,
        //             boost::asio::placeholders::bytes_transferred,
        //             writeIndex));
        // }
        // else {
            // std::cout << m_incomingBundleSize << "\n\n" << std::endl;
            // boost::endian::big_to_native_inplace(m_incomingBundleSize);
            //continue operation StartTcpReceiveBundleData only if there was no error
            if (m_tcpReceiveBuffer.size() > m_maxTcpPacketSize) { //SAFETY CHECKS ON SIZE BEFORE ALLOCATE
                LOG_FATAL(subprocess) << "TcpPacketSink::HandleTcpReceiveIncomingBundleSize(): size " << m_incomingBundleSize << " exceeds 100MB.. TCP receiving on TcpPacketSink will now stop!";
                DoTcpShutdown(); //leave in m_stateTcpReadActive = true
            }
            else {
                m_tcpReceiveBuffersCbVec[writeIndex].resize(m_tcpReceiveBuffer.size());
                boost::asio::async_read(*m_tcpSocketPtr,
                    boost::asio::buffer(m_tcpReceiveBuffersCbVec[writeIndex]),
                    boost::bind(&TcpPacketSink::HandleTcpReceiveBundleData, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred,
                        writeIndex));
            }
        // }
    }
    else if (error == boost::asio::error::eof) {
        LOG_INFO(subprocess) << "Tcp connection closed cleanly by peer";
        DoTcpShutdown();
    }
    else if (error != boost::asio::error::operation_aborted) {
        LOG_ERROR(subprocess) << "TcpPacketSink::HandleTcpReceiveIncomingBundleSize: " << error.message();
    }
}

void TcpPacketSink::HandleTcpReceiveBundleData(const boost::system::error_code & error, std::size_t bytesTransferred, unsigned int writeIndex) {
    if (!error) {
        // if (bytesTransferred == m_incomingBundleSize) {
            m_tcpReceiveBytesTransferredCbVec[writeIndex] = bytesTransferred;
            m_mutexCb.lock();
            m_circularIndexBuffer.CommitWrite(); //write complete at this point
            m_mutexCb.unlock();
            m_conditionVariableCb.notify_one();
            m_telemetry.m_totalBundleBytesReceived += bytesTransferred;
            m_telemetry.m_totalStcpBytesReceived += bytesTransferred;
            ++(m_telemetry.m_totalBundlesReceived);
            m_stateTcpReadActive = false; //must be false before calling TryStartTcpReceive
            TryStartTcpReceive(); //restart operation only if there was no error
        // }
        // else {
            // LOG_ERROR(subprocess) << "TcpPacketSink::HandleTcpReceiveBundleData: bytesTransferred ("
                // << bytesTransferred << ") != m_incomingBundleSize (" << m_incomingBundleSize << ")";
        // }
    }
    else if (error == boost::asio::error::eof) {
        LOG_INFO(subprocess) << "Tcp connection closed cleanly by peer";
        DoTcpShutdown();
    }
    else if (error != boost::asio::error::operation_aborted) {
        LOG_ERROR(subprocess) << "TcpPacketSink::HandleTcpReceiveBundleData: " << error.message();
    }
}



void TcpPacketSink::PopCbThreadFunc() {
    ThreadNamer::SetThisThreadName("TcpPacketSinkCbReader");

    while (true) { //keep thread alive if running or cb not empty, i.e. "while (m_running || (m_circularIndexBuffer.GetIndexForRead() != CIRCULAR_INDEX_BUFFER_EMPTY))"
        unsigned int consumeIndex = m_circularIndexBuffer.GetIndexForRead(); //store the volatile
        boost::asio::post(m_tcpSocketIoServiceRef, boost::bind(&TcpPacketSink::TryStartTcpReceive, this)); //keep this a thread safe operation by letting ioService thread run it
        if (consumeIndex == CIRCULAR_INDEX_BUFFER_EMPTY) { //if empty
            //try again, but with the mutex
            boost::mutex::scoped_lock lock(m_mutexCb);
            consumeIndex = m_circularIndexBuffer.GetIndexForRead(); //store the volatile
            if (consumeIndex == CIRCULAR_INDEX_BUFFER_EMPTY) { //if empty again (lock mutex (above) before checking condition)
                if (!m_running) { //m_running is mutex protected, if it stopped running, exit the thread (lock mutex (above) before checking condition)
                    break; //thread stopping criteria (empty and not running)
                }
                m_conditionVariableCb.wait(lock); // call lock.unlock() and blocks the current thread
                //thread is now unblocked, and the lock is reacquired by invoking lock.lock()
                continue;
            }
        }
        m_wholePacketReadyCallback(m_tcpReceiveBuffersCbVec[consumeIndex]);
        
        m_circularIndexBuffer.CommitRead();
    }

    LOG_INFO(subprocess) << "TcpPacketSink Circular buffer reader thread exiting";

}

void TcpPacketSink::DoTcpShutdown() {
    boost::asio::post(m_tcpSocketIoServiceRef, boost::bind(&TcpPacketSink::HandleSocketShutdown, this));
}

void TcpPacketSink::HandleSocketShutdown() {
    //final code to shut down tcp sockets
    if (m_tcpSocketPtr) {
        if (m_tcpSocketPtr->is_open()) {
            try {
                LOG_INFO(subprocess) << "shutting down TcpPacketSink TCP socket..";
                m_tcpSocketPtr->shutdown(boost::asio::socket_base::shutdown_type::shutdown_both);
            }
            catch (const boost::system::system_error & e) {
                LOG_ERROR(subprocess) << "TcpPacketSink::HandleSocketShutdown: " << e.what();
            }
            try {
                LOG_INFO(subprocess) << "closing TcpPacketSink TCP socket socket..";
                m_tcpSocketPtr->close();
            }
            catch (const boost::system::system_error & e) {
                LOG_ERROR(subprocess) << "TcpPacketSink::HandleSocketShutdown: " << e.what();
            }
        }
        LOG_INFO(subprocess) << "deleting TcpPacketSink TCP Socket";
        if (m_tcpSocketPtr.use_count() != 1) {
            LOG_ERROR(subprocess) << "m_tcpSocketPtr.use_count() != 1";
        }
        m_tcpSocketPtr = std::shared_ptr<boost::asio::ip::tcp::socket>();
    }
    m_safeToDelete = true;
    if (m_notifyReadyToDeleteCallback) {
        m_notifyReadyToDeleteCallback();
    }
}

bool TcpPacketSink::ReadyToBeDeleted() {
    return m_safeToDelete;
}



