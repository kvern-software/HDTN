#include "MediaSink.h"
#include "MediaSinkRunner.h"

#include "SignalHandler.h"
#include "Logger.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include "Uri.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

void MediaSinkRunner::MonitorExitKeypressThreadFunction() {
    LOG_INFO(subprocess) << "Keyboard Interrupt.. exiting";
    m_runningFromSigHandler = false; //do this first
}


MediaSinkRunner::MediaSinkRunner() {}
MediaSinkRunner::~MediaSinkRunner() {}

void placeholder(buffer * buf)
{
    
}

bool MediaSinkRunner::Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler) {
    //scope to ensure clean exit before return 0
    {
        running = true;
        m_runningFromSigHandler = true;
        SignalHandler sigHandler(boost::bind(&MediaSinkRunner::MonitorExitKeypressThreadFunction, this));
        uint32_t processingLagMs;
        uint64_t maxBundleSizeBytes;
        InductsConfig_ptr inductsConfigPtr;
        OutductsConfig_ptr outductsConfigPtr;
        cbhe_eid_t myEid;
        bool isAcsAware;

        bool enableGui = false;
        uint8_t local_frame_queue_size = 0;
        
        boost::program_options::options_description desc("Allowed options");
        try {
            desc.add_options()
                ("help", "Produce help message.")
                ("simulate-processing-lag-ms", boost::program_options::value<uint32_t>()->default_value(0), "Extra milliseconds to process bundle (testing purposes).")
                ("inducts-config-file", boost::program_options::value<boost::filesystem::path>()->default_value(""), "Inducts Configuration File.")
                ("my-uri-eid", boost::program_options::value<std::string>()->default_value("ipn:2.1"), "BpSink Eid.")
                ("custody-transfer-outducts-config-file", boost::program_options::value<boost::filesystem::path>()->default_value(""), "Outducts Configuration File for custody transfer (use custody if present).")
                ("acs-aware-bundle-agent", "Custody transfer should support Aggregate Custody Signals if valid CTEB present.")
                ("max-rx-bundle-size-bytes", boost::program_options::value<uint64_t>()->default_value(10000000), "Max bundle size bytes to receive (default=10MB).")
                ("enable-gui", boost::program_options::value<bool>()->default_value(false), "Enable viewing of received data")
                ("local-frame-queue-size", boost::program_options::value<uint8_t>()->default_value(5), "Number of frames to keep in queue before sending a packet")
                ;

            boost::program_options::variables_map vm;
            boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc, boost::program_options::command_line_style::unix_style | boost::program_options::command_line_style::case_insensitive), vm);
            boost::program_options::notify(vm);

            if (vm.count("help")) {
                LOG_INFO(subprocess) << desc;
                return false;
            }
            const std::string myUriEid = vm["my-uri-eid"].as<std::string>();
            if (!Uri::ParseIpnUriString(myUriEid, myEid.nodeId, myEid.serviceId)) {
                LOG_ERROR(subprocess) << "error: bad bpsink uri string: " << myUriEid;
                return false;
            }

            const boost::filesystem::path configFileNameInducts = vm["inducts-config-file"].as<boost::filesystem::path>();
            if (!configFileNameInducts.empty()) {
                inductsConfigPtr = InductsConfig::CreateFromJsonFilePath(configFileNameInducts);
                if (!inductsConfigPtr) {
                    LOG_ERROR(subprocess) << "error loading config file: " << configFileNameInducts;
                    return false;
                }
                std::size_t numBpSinkInducts = inductsConfigPtr->m_inductElementConfigVector.size();
                if (numBpSinkInducts != 1) {
                    LOG_ERROR(subprocess) << "number of bp sink inducts is not 1: got " << numBpSinkInducts;
                }
            }
            else {
                LOG_WARNING(subprocess) << "notice: bpsink has no induct... bundle data will have to flow in through a bidirectional tcpcl outduct";
            }

            //create outduct for custody signals
            const boost::filesystem::path outductsConfigFileName = vm["custody-transfer-outducts-config-file"].as<boost::filesystem::path>();
            if (!outductsConfigFileName.empty()) {
                outductsConfigPtr = OutductsConfig::CreateFromJsonFilePath(outductsConfigFileName);
                if (!outductsConfigPtr) {
                    LOG_ERROR(subprocess) << "error loading config file: " << outductsConfigFileName;
                    return false;
                }
                std::size_t numBpSinkOutducts = outductsConfigPtr->m_outductElementConfigVector.size();
                if (numBpSinkOutducts != 1) {
                    LOG_ERROR(subprocess) << "number of bpsink outducts is not 1: got " << numBpSinkOutducts;
                }
            }
            isAcsAware = (vm.count("acs-aware-bundle-agent"));

            processingLagMs = vm["simulate-processing-lag-ms"].as<uint32_t>();
            maxBundleSizeBytes = vm["max-rx-bundle-size-bytes"].as<uint64_t>();

            enableGui = vm["enable-gui"].as<bool>();
            local_frame_queue_size = vm["local-frame-queue-size"].as<uint8_t>();

        }
        catch (boost::bad_any_cast & e) {
            LOG_ERROR(subprocess) << "invalid data error: " << e.what();
            LOG_ERROR(subprocess) << desc;
            return false;
        }
        catch (std::exception& e) {
            LOG_ERROR(subprocess) << e.what();
            return false;
        }
        catch (...) {
            LOG_ERROR(subprocess) << "Exception of unknown type!";
            return false;
        }

        
         // Start media context
        DtnContext dtnContext; // TODO figure out rtp overhead 

        // context can have multiple sessions
        std::shared_ptr<DtnSession> dtnSession = dtnContext.CreateDtnSession(RTP_RECV_ONLY);
        
        // spawn a stream in the session. a session can have receiving stream, sending stream, or both
        std::shared_ptr<DtnMediaStream> dtnMediaStream = dtnSession->CreateDtnMediaStream(RTP_FORMAT_H265);

        // configure the stream // TODO pass in from command line
        dtnMediaStream->Init(RTP_FORMAT_H265, local_frame_queue_size,  "192.168.1.132", "127.0.0.1", 55001, 55000, 5000000); // TODO pass in parameters from command line

        MediaSink mediaSink(dtnMediaStream);
        mediaSink.Init(inductsConfigPtr, outductsConfigPtr, isAcsAware, myEid, processingLagMs, maxBundleSizeBytes);

        MediaGui mediaGui;
        if (enableGui)
            mediaGui.Init();
         
        

        if (useSignalHandler) 
            sigHandler.Start(false);
        

        LOG_INFO(subprocess) << "Up and running";
        while (running && m_runningFromSigHandler) {
            if (useSignalHandler) 
                sigHandler.PollOnce();
            
            if (dtnMediaStream->GetIncomingFrameQueuePtr()->GetCurrentQueueSize() > 0) {
                // load new image
                mediaGui.LoadTextureFromVideoDevice(dtnMediaStream->GetIncomingFrameQueuePtr()->GetNextFrame().payload.start, 
                        dtnMediaStream->GetIncomingFrameQueuePtr()->GetNextFrame().payload.length);
                
                dtnMediaStream->GetIncomingFrameQueuePtr()->PopFrame();
            }

            if (enableGui) {
                if (mediaGui.should_close == false) {
                    mediaGui.NewFrame();
                    mediaGui.DisplayImage();
                    mediaGui.ExitButton();
                    mediaGui.Render();
                } else {
                    running = false;
                }
            } else {
                boost::this_thread::sleep(boost::posix_time::millisec(250));
            }
        }


        LOG_INFO(subprocess) << "Exiting cleanly..";
        mediaSink.Stop();

        mediaGui.Close();

        m_totalBytesRx = mediaSink.m_FinalStatsBpSink.m_totalBytesRx;
        m_receivedCount = mediaSink.m_FinalStatsBpSink.m_receivedCount;
        m_duplicateCount = mediaSink.m_FinalStatsBpSink.m_duplicateCount;
        this->m_FinalStatsBpSink = mediaSink.m_FinalStatsBpSink;
    }
    LOG_INFO(subprocess) << "Exited cleanly";
    return true;

}
