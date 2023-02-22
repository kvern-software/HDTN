#include "MediaSink.h"
#include "MediaSinkRunner.h"

#include "SignalHandler.h"
#include "Logger.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include "Uri.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

MediaSinkRunner::MediaSinkRunner() {}
MediaSinkRunner::~MediaSinkRunner() {}


void MediaSinkRunner::MonitorExitKeypressThreadFunction() {
    LOG_INFO(subprocess) << "Keyboard Interrupt.. exiting";
    m_runningFromSigHandler = false; //do this first
}

bool MediaSinkRunner::Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler) {   
    //scope to ensure clean exit before return 0
    {
        running = true;
        m_runningFromSigHandler = true;
        SignalHandler sigHandler(boost::bind(&MediaSinkRunner::MonitorExitKeypressThreadFunction, this));
        InductsConfig_ptr inductsConfigPtr;
        OutductsConfig_ptr outductsConfigPtr;
        cbhe_eid_t myEid;
        bool isAcsAware;
        boost::filesystem::path saveDirectory;
        uint64_t maxBundleSizeBytes;
        bool gui_enabled;

        boost::program_options::options_description desc("Allowed options");
        try {
            desc.add_options()
                ("help", "Produce help message.")
                ("save-directory", boost::program_options::value<boost::filesystem::path>()->default_value(""), "Directory to save file(s) to.  Empty=>DoNotSaveToDisk")
                ("inducts-config-file", boost::program_options::value<std::string>()->default_value(""), "Inducts Configuration File.")
                ("my-uri-eid", boost::program_options::value<std::string>()->default_value("ipn:2.1"), "BpReceiveFile Eid.")
                ("custody-transfer-outducts-config-file", boost::program_options::value<std::string>()->default_value(""), "Outducts Configuration File for custody transfer (use custody if present).")
                ("acs-aware-bundle-agent", "Custody transfer should support Aggregate Custody Signals if valid CTEB present.")
                ("max-rx-bundle-size-bytes", boost::program_options::value<uint64_t>()->default_value(10000000), "Max bundle size bytes to receive (default=10MB).")
                ("gui", boost::program_options::value<bool>()->default_value(true), "Enable graphical user interface")
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
                LOG_ERROR(subprocess) << "bad BpReceiveFile uri string: " << myUriEid;
                return false;
            }

            const std::string configFileNameInducts = vm["inducts-config-file"].as<std::string>();
            if (configFileNameInducts.length()) {
                inductsConfigPtr  = InductsConfig::CreateFromJsonFilePath(configFileNameInducts);
                // inductsConfigPtr = InductsConfig::CreateFromJsonFile(configFileNameInducts);

                if (!inductsConfigPtr) {
                    LOG_ERROR(subprocess) << "error loading config file: " << configFileNameInducts;
                    return false;
                }
                std::size_t numInducts = inductsConfigPtr->m_inductElementConfigVector.size();
                if (numInducts != 1) {
                    LOG_ERROR(subprocess) << "number of BpReceiveFile inducts is not 1: got " << numInducts;
                }
            }
            else {
                LOG_WARNING(subprocess) << "notice: BpReceiveFile has no induct... bundle data will have to flow in through a bidirectional tcpcl outduct";
            }

            //create outduct for custody signals
            const std::string outductsConfigFileName = vm["custody-transfer-outducts-config-file"].as<std::string>();
            if (outductsConfigFileName.length()) {
                outductsConfigPtr = OutductsConfig::CreateFromJsonFilePath(outductsConfigFileName);
                if (!outductsConfigPtr) {
                    LOG_ERROR(subprocess) << "error loading config file: " << outductsConfigFileName;
                    return false;
                }
                std::size_t numOutducts = outductsConfigPtr->m_outductElementConfigVector.size();
                if (numOutducts != 1) {
                    LOG_ERROR(subprocess) << "number of BpReceiveFile outducts is not 1: got " << numOutducts;
                }
            }
            isAcsAware = (vm.count("acs-aware-bundle-agent"));
            saveDirectory = vm["save-directory"].as<boost::filesystem::path>();
            maxBundleSizeBytes = vm["max-rx-bundle-size-bytes"].as<uint64_t>();
            gui_enabled = vm["gui"].as<bool>();
        }
        catch (boost::bad_any_cast & e) {
            LOG_ERROR(subprocess) << "invalid data error: " << e.what() << "\n";
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

        // End HDTN boiler plate code

        // Start core media app functionality 
        LOG_INFO(subprocess) << "starting..";

        MediaSink mediaSink(saveDirectory);
        mediaSink.Init(inductsConfigPtr, outductsConfigPtr, isAcsAware, myEid, 0, maxBundleSizeBytes);

        if (useSignalHandler) {
            sigHandler.Start(false);
        }
        
        if (!gui_enabled) {
            LOG_INFO(subprocess) << "Starting without GUI";
        }
        
        LOG_INFO(subprocess) << "Up and running";


        // main loop  
        while (running && m_runningFromSigHandler && !mediaSink.mediaApp.should_close) {
            if (gui_enabled) {
                mediaSink.mediaApp.NewFrame();
                if (mediaSink.mediaApp.should_update_image == true) 
                    mediaSink.mediaApp.UpdateImage(mediaSink.nextFileFullPathFileName);
            
                mediaSink.mediaApp.DisplayImage();
                mediaSink.mediaApp.ExitButton();

                mediaSink.mediaApp.Render();
            }


            if (useSignalHandler) {
                sigHandler.PollOnce();
            }
        }

        LOG_INFO(subprocess) << "Exiting cleanly..";
        mediaSink.mediaApp.Close(); // clean up gui

        mediaSink.Stop(); // clean up networking

        //safe to get any stats now if needed
    }
    LOG_INFO(subprocess) << "Exited cleanly";   

    return true;
}

