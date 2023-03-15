/**
 * @file BundleStorageManagerAsio.cpp
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright © 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 */

#ifndef _WIN32
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <fcntl.h>
#endif

#include "BundleStorageManagerAsio.h"
#include <string>
#include <boost/filesystem.hpp>
#include <memory>
#include <boost/make_unique.hpp>
#include "ThreadNamer.h"
#ifdef _WIN32
#include <windows.h> //must be included after boost
#endif

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::storage;

BundleStorageManagerAsio::BundleStorageManagerAsio() : BundleStorageManagerAsio("storageConfig.json") {}

BundleStorageManagerAsio::BundleStorageManagerAsio(const boost::filesystem::path& jsonConfigFilePath) :
    BundleStorageManagerAsio(StorageConfig::CreateFromJsonFilePath(jsonConfigFilePath))
{
    if (!m_storageConfigPtr) {
        LOG_ERROR(subprocess) << "cannot open storage json config file: " << jsonConfigFilePath;
        return;
    }
}

BundleStorageManagerAsio::BundleStorageManagerAsio(const StorageConfig_ptr & storageConfigPtr) :
    BundleStorageManagerBase(storageConfigPtr),

    m_workPtr(boost::make_unique< boost::asio::io_service::work>(m_ioService)),
    m_asioHandlePtrsVec(M_NUM_STORAGE_DISKS),
    m_diskOperationInProgressVec(M_NUM_STORAGE_DISKS)
{



}

BundleStorageManagerAsio::~BundleStorageManagerAsio() {
    if (m_ioServiceThreadPtr) {
        try {
            m_workPtr.reset(); //erase the work object (destructor is thread safe) so that io_service thread will exit when it runs out of work 
            m_ioServiceThreadPtr->join();
            m_ioServiceThreadPtr.reset(); //delete it
        }
        catch (const boost::thread_resource_error&) {
            LOG_ERROR(subprocess) << "error stopping BundleStorageManagerAsio io_service";
        }
    }

    for (unsigned int diskId = 0; diskId < M_NUM_STORAGE_DISKS; ++diskId) {
        if (m_asioHandlePtrsVec[diskId]) {
            m_asioHandlePtrsVec[diskId]->close();
            m_asioHandlePtrsVec[diskId].reset(); //delete it
        }
    }

}

void BundleStorageManagerAsio::Start() {
    if (m_storageConfigPtr) {
        for (unsigned int diskId = 0; diskId < M_NUM_STORAGE_DISKS; ++diskId) {
            const boost::filesystem::path& filePath = m_filePathsVec[diskId];
            const boost::filesystem::path::value_type* filePathCstr = filePath.c_str();
            LOG_INFO(subprocess) << ((m_successfullyRestoredFromDisk) ? "reopening " : "creating ") << filePath;
#ifdef _WIN32
            //
            //https://docs.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o
            //In synchronous file I/O, a thread starts an I/O operation and immediately enters a wait state until the I/O request has completed.
            //A thread performing asynchronous file I/O sends an I/O request to the kernel by calling an appropriate function.
            //If the request is accepted by the kernel, the calling thread continues processing another job until the kernel signals to
            //the thread that the I/O operation is complete. It then interrupts its current job and processes the data from the I/O operation as necessary.
            //Asynchronous I/O is also referred to as overlapped I/O.
            HANDLE hFile = CreateFileW(filePathCstr,                // name of the file
                GENERIC_READ | GENERIC_WRITE,          // open for reading and writing
                0,                      // do not share
                NULL,                   // default security
                //CREATE_ALWAYS : Creates a new file, always. If the specified file exists and is writable, the function overwrites the file
                //OPEN_EXISTING : Opens a file or device, only if it exists.  If the specified file or device does not exist, the function fails and the last - error code is set to ERROR_FILE_NOT_FOUND(2).
                (m_successfullyRestoredFromDisk) ? OPEN_EXISTING : CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // normal file
                NULL);                  // no attr. template

            if (hFile == INVALID_HANDLE_VALUE) {
                LOG_ERROR(subprocess) << "error opening " << filePath;
                return;
            }
            //
            //FILE * fileHandle = (m_successfullyRestoredFromDisk) ? fopen(filePath, "r+bR") : fopen(filePath, "w+bR");
            m_asioHandlePtrsVec[diskId] = boost::make_unique<boost::asio::windows::random_access_handle>(m_ioService, hFile);
#else
            int file_desc = open(filePathCstr, (m_successfullyRestoredFromDisk) ? (O_RDWR|O_LARGEFILE) : (O_CREAT|O_RDWR|O_TRUNC|O_LARGEFILE), DEFFILEMODE);
            if(file_desc < 0) {
                LOG_ERROR(subprocess) << "error opening " << filePath;
                return;
            }
            m_asioHandlePtrsVec[diskId] = boost::make_unique<boost::asio::posix::stream_descriptor>(m_ioService, file_desc);
#endif
            m_diskOperationInProgressVec[diskId] = false;
        }
        m_ioServiceThreadPtr = boost::make_unique<boost::thread>(boost::bind(&boost::asio::io_service::run, &m_ioService));
        ThreadNamer::SetIoServiceThreadName(m_ioService, "ioServiceStorageAsio");
    }
}

void BundleStorageManagerAsio::TryDiskOperation_Consume_NotThreadSafe(const unsigned int diskId) {
    if (!m_diskOperationInProgressVec[diskId]) {
        CircularIndexBufferSingleProducerSingleConsumerConfigurable & cb = m_circularIndexBuffersVec[diskId];
        const unsigned int consumeIndex = cb.GetIndexForRead(); //store the volatile

        if (consumeIndex != CIRCULAR_INDEX_BUFFER_EMPTY) { //if not empty
            m_diskOperationInProgressVec[diskId] = true;

            segment_id_t * const circularBufferSegmentIdsPtr = &m_circularBufferSegmentIdsPtr[diskId * CIRCULAR_INDEX_BUFFER_SIZE];

            const segment_id_t segmentId = circularBufferSegmentIdsPtr[consumeIndex];
            volatile boost::uint8_t * const readFromStorageDestPointer = m_circularBufferReadFromStoragePointers[diskId * CIRCULAR_INDEX_BUFFER_SIZE + consumeIndex];

            const bool isWriteToDisk = (readFromStorageDestPointer == NULL);
            if (segmentId == SEGMENT_ID_LAST) {
                LOG_ERROR(subprocess) << "error segmentId is last";
                //continue;
            }

            const boost::uint64_t offsetBytes = static_cast<boost::uint64_t>(segmentId / M_NUM_STORAGE_DISKS) * SEGMENT_SIZE;

#ifdef _WIN32

#else
            lseek64(m_asioHandlePtrsVec[diskId]->native_handle(), offsetBytes, SEEK_SET);
#endif

            if (isWriteToDisk) {
                boost::uint8_t * const circularBufferBlockDataPtr = &m_circularBufferBlockDataPtr[diskId * CIRCULAR_INDEX_BUFFER_SIZE * SEGMENT_SIZE];
                boost::uint8_t * const data = &circularBufferBlockDataPtr[consumeIndex * SEGMENT_SIZE]; //expected data for testing when reading
#ifdef _WIN32
                boost::asio::async_write_at(*m_asioHandlePtrsVec[diskId], offsetBytes,
#else
                boost::asio::async_write(*m_asioHandlePtrsVec[diskId],
#endif
                    boost::asio::buffer(data, SEGMENT_SIZE),
                    boost::bind(&BundleStorageManagerAsio::HandleDiskOperationCompleted, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred,
                        diskId, consumeIndex, false));

            }
            else { //read from disk
#ifdef _WIN32
                boost::asio::async_read_at(*m_asioHandlePtrsVec[diskId], offsetBytes,
#else
                boost::asio::async_read(*m_asioHandlePtrsVec[diskId],
#endif
                    boost::asio::buffer((void*)readFromStorageDestPointer, SEGMENT_SIZE),
                    boost::bind(&BundleStorageManagerAsio::HandleDiskOperationCompleted, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred,
                        diskId, consumeIndex, true));
            }
        }
    }
}

//virtual function to be called immediately after a disk's circular buffer CommitWrite();
void BundleStorageManagerAsio::CommitWriteAndNotifyDiskOfWorkToDo_ThreadSafe(const unsigned int diskId) {
    CircularIndexBufferSingleProducerSingleConsumerConfigurable& cb = m_circularIndexBuffersVec[diskId];
    cb.CommitWrite();
    boost::asio::post(m_ioService, boost::bind(&BundleStorageManagerAsio::TryDiskOperation_Consume_NotThreadSafe, this, diskId));
}


void BundleStorageManagerAsio::HandleDiskOperationCompleted(const boost::system::error_code& error, std::size_t bytes_transferred, const unsigned int diskId, const unsigned int consumeIndex, const bool wasReadOperation) {
    m_diskOperationInProgressVec[diskId] = false;
    if (error) {
        LOG_ERROR(subprocess) << "error in BundleStorageManagerMT::HandleDiskOperationCompleted: " << error.message();
    }
    else if (bytes_transferred != SEGMENT_SIZE) {
        LOG_ERROR(subprocess) << "error in BundleStorageManagerMT::HandleDiskOperationCompleted: bytes_transferred(" << bytes_transferred << ") != SEGMENT_SIZE(" << SEGMENT_SIZE << ")";
    }
    else {
        volatile bool junk;
        volatile bool * const isReadCompletedPointer = (wasReadOperation) ?
            m_circularBufferIsReadCompletedPointers[diskId * CIRCULAR_INDEX_BUFFER_SIZE + consumeIndex] : &junk;
        CircularIndexBufferSingleProducerSingleConsumerConfigurable & cb = m_circularIndexBuffersVec[diskId];
        m_mutexMainThread.lock();
        *isReadCompletedPointer = true;
        cb.CommitRead();
        m_mutexMainThread.unlock();
        m_conditionVariableMainThread.notify_one();
        TryDiskOperation_Consume_NotThreadSafe(diskId);
    }
}
