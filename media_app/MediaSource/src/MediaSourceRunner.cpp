#include "MediaSource.h"
#include "VideoDriver.h"

#include "SignalHandler.h"
#include "Logger.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include "Uri.h"
#include "OutductsConfig.h"
#include "MediaSourceRunner.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

MediaSourceRunner::MediaSourceRunner() {}
MediaSourceRunner::~MediaSourceRunner() {}


void MediaSourceRunner::MonitorExitKeypressThreadFunction() {
    LOG_INFO(subprocess) << "Keyboard Interrupt.. exiting";
    m_runningFromSigHandler = false; //do this first
}


static void DurationEndedThreadFunction(const boost::system::error_code& e, volatile bool * running) {
    if (e != boost::asio::error::operation_aborted) {
        // Timer was not cancelled, take necessary action.
        LOG_INFO(subprocess) << "Reached duration.. exiting";
    }
    else {
        LOG_ERROR(subprocess) << "Unknown error occurred in DurationEndedThreadFunction " << e.message();
    }
    *running = false;
}

bool MediaSourceRunner::Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler) {
    //scope to ensure clean exit before return 0
    {
        running = true;
        m_runningFromSigHandler = true;
        SignalHandler sigHandler(boost::bind(&MediaSourceRunner::MonitorExitKeypressThreadFunction, this));
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
        bool gui_enabled;
        bool save_local_copy;
        boost::filesystem::path video_device;
        uint64_t frame_width, frame_height = 0;
        uint64_t frames_per_second = 0;

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
                    ("video-device",  boost::program_options::value<boost::filesystem::path>()->default_value(""), "Path to camera device Empty=>No Camera")
                    ("gui", boost::program_options::value<bool>()->default_value(true), "Enable graphical user interface")
                    ("save-local-copy", boost::program_options::value<bool>()->default_value(false), "Save local copy of media to disk")
                    ("frame-width", boost::program_options::value<uint64_t>()->default_value(1920), "Camera resolution in X axis")
                    ("frame-height", boost::program_options::value<uint64_t>()->default_value(1080), "Camera resolation in Y axis")
                    ("frames-per-second", boost::program_options::value<uint64_t>()->default_value(60), "Number of buffered frames kept in memory")
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
                    // OutductsConfig::CreateFromJsonFilePath();
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
                bundleRate = vm["bundle-rate"].as<uint32_t>();
                durationSeconds = vm["duration"].as<uint32_t>();
                myCustodianServiceId = vm["my-custodian-service-id"].as<uint64_t>();
                bundleSendTimeoutSeconds = vm["bundle-send-timeout-seconds"].as<unsigned int>();
                bundleLifetimeMilliseconds = vm["bundle-lifetime-milliseconds"].as<uint64_t>();
                video_device = vm["video-device"].as<boost::filesystem::path>();
                gui_enabled = vm["gui"].as<bool>();
                save_local_copy = vm["save-local-copy"].as<bool>();
                frame_width = vm["frame-width"].as<uint64_t>();
                frame_height = vm["frame-height"].as<uint64_t>();
                frames_per_second = vm["frames-per-second"].as<uint64_t>();
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


        LOG_INFO(subprocess) << "starting MediaSource..";
        LOG_INFO(subprocess) << "Sending Bundles from MediaSource Node " << myEid.nodeId << " to final Destination Node " << finalDestEid.nodeId; 
        MediaSource mediaSource(bundleSizeBytes);
        
        if (!video_device.empty()) {
            // initialize video driver
            mediaSource.videoDriverPtr->device = video_device;
            mediaSource.video_driver_enabled = true;
            mediaSource.videoDriverPtr->OpenFD();
            mediaSource.videoDriverPtr->CheckDeviceCapability();
            mediaSource.videoDriverPtr->SetImageFormat(DEFAULT_VIDEO_CAPTURE, frame_width, frame_height, 
                    DEFAULT_PIXEL_FORMAT, DEFAULT_FIELD);
            mediaSource.videoDriverPtr->SetFramerate(frames_per_second);
            mediaSource.videoDriverPtr->SetCaptureMode(FIFO); // todo get input come command line
            mediaSource.videoDriverPtr->RequestBuffer(V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP);
            mediaSource.videoDriverPtr->AllocateLocalBuffers();    
            // mediaSource.videoDriverPtr->RegisterCallback(&mediaSource.mediaApp);
            mediaSource.videoDriverPtr->Start();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        } else {
            LOG_INFO(subprocess) << "started with no video driver";
        }

        mediaSource.Start(
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
        LOG_INFO(subprocess) << "running MediaSource for " << durationSeconds << " seconds";
        
        bool startedTimer = false;
        

        if (useSignalHandler) {
            sigHandler.Start(false);
        }

       
        if (!gui_enabled) {
            LOG_INFO(subprocess) << "running without a GUI";
        }

        LOG_INFO(subprocess) << "MediaSource up and running";
        
        
        while (running && m_runningFromSigHandler && mediaSource.mediaAppPtr->should_close==false) {
            if (durationSeconds) {
                if ((!startedTimer) && mediaSource.m_allOutductsReady) {
                    startedTimer = true;
                    deadlineTimer.expires_from_now(boost::posix_time::seconds(durationSeconds));
                    deadlineTimer.async_wait(boost::bind(&DurationEndedThreadFunction, boost::asio::placeholders::error, &running));
                }
                else {
                    ioService.poll_one();
                }
            }
            if (useSignalHandler) {
                sigHandler.PollOnce();
            }

            if (mediaSource.video_driver_enabled) {   
                if (save_local_copy) {
                    mediaSource.saveFileFullFilename = ("file_");
                    mediaSource.saveFileFullFilename.append(std::to_string(mediaSource.file_number));
                    mediaSource.saveFileFullFilename.append(".jpeg");
                    mediaSource.videoDriverPtr->WriteBufferToFile(mediaSource.saveFileFullFilename, DEFAULT_CHUNK_WRITE_SIZE);
                    mediaSource.file_number++;
                }
            }

            // GUI
            if (gui_enabled) {
                mediaSource.mediaAppPtr->LoadTextureFromVideoDevice(mediaSource.rawFrameBuffer.start, mediaSource.rawFrameBuffer.length);

                mediaSource.mediaAppPtr->NewFrame();
                mediaSource.mediaAppPtr->DisplayImage();
                mediaSource.mediaAppPtr->ExitButton();

                mediaSource.mediaAppPtr->Render();
            }

            
        }

        LOG_INFO(subprocess) << "VideoDriver exiting cleanly..";
        mediaSource.videoDriverPtr->Stop();
        LOG_INFO(subprocess) << "VideoDriver exited cleanly..";
        
        LOG_INFO(subprocess) << "MediaApp exiting cleanly..";
        mediaSource.mediaAppPtr->Close();
        LOG_INFO(subprocess) << "MediaApp exited cleanly..";

        
        LOG_INFO(subprocess) << "MediaSourceRunner::Run: exiting cleanly..";
        mediaSource.Stop();
        m_bundleCount = mediaSource.m_bundleCount;
        m_outductFinalStats = mediaSource.m_outductFinalStats;
    }
    LOG_INFO(subprocess) << "MediaSourceRunner::Run: exited cleanly";
    return true;

}


