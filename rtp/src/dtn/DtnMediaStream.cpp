#include "DtnMediaStream.h"

#include "SignalHandler.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include "Uri.h"
#include "Logger.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

// rtp_error_t DtnMediaStreamSource::CreateMedia(rtp_format_t fmt)
// {
//     switch (fmt) {
//         // case RTP_FORMAT_H264:
//         // {
//         //     uvgrtp::formats::h264* format_264 = new uvgrtp::formats::h264(socket_, rtp_, rce_flags_);

//         //     reception_flow_->install_aux_handler_cpp(
//         //         rtp_handler_key_,
//         //         std::bind(&uvgrtp::formats::h264::packet_handler, format_264, std::placeholders::_1, std::placeholders::_2),
//         //         std::bind(&uvgrtp::formats::h264::frame_getter, format_264, std::placeholders::_1));
//         //     m_media.reset(format_264);
//         //     break;
//         // }
//         case RTP_FORMAT_H265:
//         {
//             uvgrtp::formats::h265* format_265 = new uvgrtp::formats::h265(socket_, rtp_, rce_flags_);

//             reception_flow_->install_aux_handler_cpp(
//                 rtp_handler_key_,
//                 std::bind(&uvgrtp::formats::h265::packet_handler, format_265, std::placeholders::_1, std::placeholders::_2),
//                 std::bind(&uvgrtp::formats::h265::frame_getter, format_265, std::placeholders::_1));
//             m_media.reset(format_265);
//             break;
//         }
//     default:
//     {
//         printf("Unknown payload format\n");
//         m_media = nullptr;
//         return RTP_NOT_SUPPORTED;
//     }

//     m_media->set_fps(m_fpsNumerator, m_fpsDenominator);

// }

int DtnMediaStreamSource::Run(int argc, const char* argv[], volatile bool & running, bool useSignalHandler)
{
    running = true;
        m_runningFromSigHandler = true;
        // SignalHandler sigHandler(boost::bind(&DtnMediaStreamSource::MonitorExitKeypressThreadFunction, this));
        uint32_t bundleSizeBytes;
        uint32_t bundleRate;
        //uint32_t tcpclFragmentSize;
        uint32_t durationSeconds;
        cbhe_eid_t myEid;
        cbhe_eid_t finalDestEid;
        uint64_t myCustodianServiceId;
        OutductsConfig_ptr outductsConfigPtr;
        InductsConfig_ptr inductsConfigPtr;
        bool custodyTransferUseAcs;
        bool forceDisableCustody;
        bool useBpVersion7;
        unsigned int bundleSendTimeoutSeconds;
        uint64_t bundleLifetimeMilliseconds;
        uint64_t bundlePriority;

        boost::program_options::options_description desc("Allowed options");
        try {
                desc.add_options()
                    ("help", "Produce help message.")
                    ("bundle-size", boost::program_options::value<uint32_t>()->default_value(100), "Bundle size bytes.")
                    ("bundle-rate", boost::program_options::value<uint32_t>()->default_value(1500), "Bundle rate. (0=>as fast as possible)")
                    ("duration", boost::program_options::value<uint32_t>()->default_value(0), "Seconds to send bundles for (0=>infinity).")
                    ("my-uri-eid", boost::program_options::value<std::string>()->default_value("ipn:1.1"), "BpGen Source Node Id.")
                    ("dest-uri-eid", boost::program_options::value<std::string>()->default_value("ipn:2.1"), "BpGen sends to this final destination Eid.")
                    ("my-custodian-service-id", boost::program_options::value<uint64_t>()->default_value(0), "Custodian service ID is always 0.")
                    ("outducts-config-file", boost::program_options::value<boost::filesystem::path>()->default_value(""), "Outducts Configuration File.")
                    ("custody-transfer-inducts-config-file", boost::program_options::value<boost::filesystem::path>()->default_value(""), "Inducts Configuration File for custody transfer (use custody if present).")
                    ("custody-transfer-use-acs", "Custody transfer should use Aggregate Custody Signals instead of RFC5050.")
                    ("force-disable-custody", "Custody transfer turned off regardless of link bidirectionality.")
                    ("use-bp-version-7", "Send bundles using bundle protocol version 7.")
                    ("bundle-send-timeout-seconds", boost::program_options::value<unsigned int>()->default_value(3), "Max time to send a bundle and get acknowledgement.")
                    ("bundle-lifetime-milliseconds", boost::program_options::value<uint64_t>()->default_value(1000000), "Bundle lifetime in milliseconds.")
                    ("bundle-priority", boost::program_options::value<uint64_t>()->default_value(2), "Bundle priority. 0 = Bulk 1 = Normal 2 = Expedited")
                    ;

                boost::program_options::variables_map vm;
                boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc, boost::program_options::command_line_style::unix_style | boost::program_options::command_line_style::case_insensitive), vm);
                boost::program_options::notify(vm);

                if (vm.count("help")) {
                        LOG_INFO(subprocess) << desc;
                        return false;
                }
                forceDisableCustody = (vm.count("force-disable-custody") != 0);
                useBpVersion7 = (vm.count("use-bp-version-7") != 0);

                const std::string myUriEid = vm["my-uri-eid"].as<std::string>();
                if (!Uri::ParseIpnUriString(myUriEid, myEid.nodeId, myEid.serviceId)) {
                    LOG_ERROR(subprocess) << "error: bad bpsink uri string: " << myUriEid;
                    return false;
                }

                const std::string myFinalDestUriEid = vm["dest-uri-eid"].as<std::string>();
                if (!Uri::ParseIpnUriString(myFinalDestUriEid, finalDestEid.nodeId, finalDestEid.serviceId)) {
                    LOG_ERROR(subprocess) << "error: bad bpsink uri string: " << myFinalDestUriEid;
                    return false;
                }

                const boost::filesystem::path outductsConfigFileName = vm["outducts-config-file"].as<boost::filesystem::path>();

                if (!outductsConfigFileName.empty()) {
                    outductsConfigPtr = OutductsConfig::CreateFromJsonFilePath(outductsConfigFileName);
                    if (!outductsConfigPtr) {
                        LOG_ERROR(subprocess) << "error loading outducts config file: " << outductsConfigFileName;
                        return false;
                    }
                    std::size_t numBpGenOutducts = outductsConfigPtr->m_outductElementConfigVector.size();
                    if (numBpGenOutducts != 1) {
                        LOG_ERROR(subprocess) << "error: number of bpgen outducts is not 1: got " << numBpGenOutducts;
                    }
                }
                else {
                    LOG_WARNING(subprocess) << "notice: bpgen has no outduct... bundle data will have to flow out through a bidirectional tcpcl induct";
                }

                //create induct for custody signals
                const boost::filesystem::path inductsConfigFileName = vm["custody-transfer-inducts-config-file"].as<boost::filesystem::path>();
                if (!inductsConfigFileName.empty()) {
                    inductsConfigPtr = InductsConfig::CreateFromJsonFilePath(inductsConfigFileName);
                    if (!inductsConfigPtr) {
                        LOG_ERROR(subprocess) << "error loading induct config file: " << inductsConfigFileName;
                        return false;
                    }
                    std::size_t numBpGenInducts = inductsConfigPtr->m_inductElementConfigVector.size();
                    if (numBpGenInducts != 1) {
                        LOG_ERROR(subprocess) << "error: number of bp gen inducts for custody signals is not 1: got " << numBpGenInducts;
                    }
                }
                custodyTransferUseAcs = (vm.count("custody-transfer-use-acs"));

                bundlePriority = vm["bundle-priority"].as<uint64_t>();
                if (bundlePriority > 2) {
                    std::cerr << "Priority must be 0, 1, or 2." << std::endl;
                    return false;
                }

                bundleSizeBytes = vm["bundle-size"].as<uint32_t>();
                m_bundleSizeBytes = bundleSizeBytes;
                bundleRate = vm["bundle-rate"].as<uint32_t>();
                durationSeconds = vm["duration"].as<uint32_t>();
                myCustodianServiceId = vm["my-custodian-service-id"].as<uint64_t>();
                bundleSendTimeoutSeconds = vm["bundle-send-timeout-seconds"].as<unsigned int>();
                bundleLifetimeMilliseconds = vm["bundle-lifetime-milliseconds"].as<uint64_t>();
        }
        catch (boost::bad_any_cast & e) {
                LOG_ERROR(subprocess) << "invalid data error: " << e.what();
                LOG_ERROR(subprocess) << desc;
                return false;
        }
        catch (std::exception& e) {
                LOG_ERROR(subprocess) << "error: " << e.what();
                return false;
        }
        catch (...) {
                LOG_ERROR(subprocess) << "Exception of unknown type!";
                return false;
        }


        LOG_INFO(subprocess) << "starting BpGenAsync..";
        LOG_INFO(subprocess) << "Sending Bundles from BPGen Node " << myEid.nodeId << " to final Destination Node " << finalDestEid.nodeId; 

        // BpGenAsync bpGen(bundleSizeBytes);
        Start(
            outductsConfigPtr,
            inductsConfigPtr,
            custodyTransferUseAcs,
            myEid,
            bundleRate,
            finalDestEid,
            myCustodianServiceId,
            bundleSendTimeoutSeconds,
            bundleLifetimeMilliseconds,
            bundlePriority,
            false,
            forceDisableCustody,
            useBpVersion7
        );

        boost::asio::io_service ioService;
        boost::asio::deadline_timer deadlineTimer(ioService);
        LOG_INFO(subprocess) << "running bpgen for " << durationSeconds << " seconds";
                
        // if (useSignalHandler) {
        //     sigHandler.Start(false);
        // }

        LOG_INFO(subprocess) << "BpGenAsync up and running";
            while (running && m_runningFromSigHandler) {
                boost::this_thread::sleep(boost::posix_time::millisec(250));
               
                if (useSignalHandler) {
                    // sigHandler.PollOnce();
                }
            }

        LOG_INFO(subprocess) << "BpGenAsyncRunner::Run: exiting cleanly..";
        Stop();
        
        LOG_INFO(subprocess) << "BpGenAsyncRunner::Run: exited cleanly";
        return true;

}

void DtnMediaStreamSource::StartComponents()
{
    // frame queue first
    m_dtnFrameQueuePtr = std::make_shared<DtnFrameQueue>(m_frameQueueSize);

    // encoder
    m_dtnEncoderPtr = std::make_shared<DtnEncoder>(m_dtnFrameQueuePtr);

    // video driver
    m_videoDriverPtr = std::make_shared<VideoDriver>(boost::bind(&DtnEncoder::FrameCallback, m_dtnEncoderPtr, boost::placeholders::_1));
        // videoDriverPtr = std::make_shared<VideoDriver>(boost::bind(&DtnMediaStreamSource::FrameCallback, this, boost::placeholders::_1));

}


uint64_t DtnMediaStreamSource::GetNextPayloadLength_Step1() 
{
    std::cout << "payload length step one: " << m_dtnFrameQueuePtr->GetCurrentQueueSizeBytes() << std::endl;
    return (uint64_t) m_dtnFrameQueuePtr->GetCurrentQueueSizeBytes();
}

bool DtnMediaStreamSource::CopyPayload_Step2(uint8_t * destinationBuffer) 
{
    memcpy(destinationBuffer, &m_dtnFrameQueuePtr->GetQueue(), m_dtnFrameQueuePtr->GetCurrentQueueSizeBytes());
    std::cout << "copied out" <<std::endl;

    // clear queue
    m_dtnFrameQueuePtr->ClearQueue();
    return true;
}

bool DtnMediaStreamSource::TryWaitForDataAvailable(const boost::posix_time::time_duration& timeout) 
{
    if (m_dtnFrameQueuePtr->GetCurrentQueueSize() != m_frameQueueSize)
    {
        // wait for full queue
        return m_dtnFrameQueuePtr->GetNextQueueTimeout(timeout);
    } 
    
    // send data, queue is full
    return true;
}

bool DtnMediaStreamSource::ProcessNonAdminRecordBundlePayload(const uint8_t * data, const uint64_t size) 
{
return 0;
}
