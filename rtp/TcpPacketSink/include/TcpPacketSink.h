#pragma once

#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <memory>
#include "CircularIndexBufferSingleProducerSingleConsumerConfigurable.h"
#include "PaddedVectorUint8.h"
#include "TelemetryDefinitions.h"



class TcpPacketSink {
private:
    TcpPacketSink();
public:
    typedef boost::function<void(padded_vector_uint8_t & wholeBundleVec)> WholePacketReadyCallback_t;
    typedef boost::function<void()> NotifyReadyToDeleteCallback_t;

    TcpPacketSink(std::shared_ptr<boost::asio::ip::tcp::socket> tcpSocketPtr,
        boost::asio::io_service & tcpSocketIoServiceRef,
        const WholePacketReadyCallback_t & wholePacketReadyCallback,
        const unsigned int numCircularBufferVectors,
        const uint32_t maxBundleSizeBytes,
        const NotifyReadyToDeleteCallback_t & notifyReadyToDeleteCallback = NotifyReadyToDeleteCallback_t());
    ~TcpPacketSink();

    bool ReadyToBeDeleted();
private:

    void TryStartTcpReceive();
    void HandleTcpReceiveIncomingBundleSize(const boost::system::error_code & error, std::size_t bytesTransferred, const unsigned int writeIndex);
    void HandleTcpReceiveBundleData(const boost::system::error_code & error, std::size_t bytesTransferred, unsigned int writeIndex);
    void PopCbThreadFunc();
    void DoTcpShutdown();
    void HandleSocketShutdown();

public:
    StcpInductConnectionTelemetry_t m_telemetry;
private:
    
    const WholePacketReadyCallback_t m_wholePacketReadyCallback;
    const NotifyReadyToDeleteCallback_t m_notifyReadyToDeleteCallback;

    
    std::shared_ptr<boost::asio::ip::tcp::socket> m_tcpSocketPtr;
    boost::asio::io_service & m_tcpSocketIoServiceRef;



    const unsigned int m_numCircularBufferElements;
    uint32_t m_maxTcpPacketSize;
    padded_vector_uint8_t m_tcpReceiveBuffer;
    CircularIndexBufferSingleProducerSingleConsumerConfigurable m_circularIndexBuffer;
    std::vector<padded_vector_uint8_t > m_tcpReceiveBuffersCbVec;
    std::vector<std::size_t> m_tcpReceiveBytesTransferredCbVec;
    boost::condition_variable m_conditionVariableCb;
    boost::mutex m_mutexCb;
    std::unique_ptr<boost::thread> m_threadCbReaderPtr;
    bool m_stateTcpReadActive;
    bool m_printedCbTooSmallNotice;
    volatile bool m_running;
    volatile bool m_safeToDelete;
    uint32_t m_incomingBundleSize = 0;
};
