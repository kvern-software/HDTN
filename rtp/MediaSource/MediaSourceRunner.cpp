#include "MediaSource.h"

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

        // END BOILER PLATE CODE ////////////////////////////////////////////////////////////
        // END BOILER PLATE CODE ////////////////////////////////////////////////////////////
        // END BOILER PLATE CODE ////////////////////////////////////////////////////////////
        // END BOILER PLATE CODE ////////////////////////////////////////////////////////////
        // END BOILER PLATE CODE ////////////////////////////////////////////////////////////
        // END BOILER PLATE CODE ////////////////////////////////////////////////////////////

        // start our bundler first
        MediaSource mediaSource(bundleSizeBytes); 
        
        // Start media context
        DtnContext dtnContext(bundleSizeBytes - 1000); // TODO figure out rtp overhead 

        // context can have multiple sessions
        std::shared_ptr<DtnSession> dtnSession = dtnContext.CreateDtnSession();
        
        // spawn a stream in the session. a session can have receiving stream, sending stream, or both
        std::shared_ptr<DtnMediaStream> dtnMediaStream = dtnSession->CreateDtnMediaStream(RTP_FORMAT_H265, 0);

        // configure the stream // TODO pass in from command line
        dtnMediaStream->Init(RTP_FORMAT_H265, frames_per_second, 1, 30, "127.0.0.1", "192.168.1.100", 55000, 55001); // TODO pass in parameters from command line

        // create a video driver to provide frames to a stream. hook the stream push frame into the video driver as a means of delivering data (uses FIFO queue)
        std::shared_ptr<VideoDriver> videoDriver = std::make_shared<VideoDriver>(boost::bind(&DtnMediaStream::PushFrame, dtnMediaStream.get(), boost::placeholders::_1));
        videoDriver->Init("/dev/video0", frame_width, frame_height, 30); // initial camera parameters and ensure we have valid camera // TODO pass in from command line 
        videoDriver->Start(); // create and start buffer filling thread

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

        LOG_INFO(subprocess) << "MediaSource up and running";
        
        // MAIN LOOP //////////////////////////////////////////
        while (running && m_runningFromSigHandler) {
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

            // LOOP CODE HERE

            // sleep?

            
        }

    
        
        LOG_INFO(subprocess) << "MediaSourceRunner::Run: exiting cleanly..";
        mediaSource.Stop();
        m_bundleCount = mediaSource.m_bundleCount;
        m_outductFinalStats = mediaSource.m_outductFinalStats;
    }
    LOG_INFO(subprocess) << "MediaSourceRunner::Run: exited cleanly";
    return true;

}

