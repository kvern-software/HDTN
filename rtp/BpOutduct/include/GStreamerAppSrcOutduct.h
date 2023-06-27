
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/smart_ptr/make_unique.hpp>
#include <boost/thread/thread.hpp>

#include "DtnUtil.h"
#include "DtnRtpFrame.h"
#include "PaddedVectorUint8.h"

#define SAMPLE_RATE 90000
#define DEFAULT_NUM_CIRC_BUFFERS 5000

#define GST_HDTN_OUTDUCT_SOCKET_PATH "/tmp/hdtn_gst_shm_outduct"

typedef boost::function<void(padded_vector_uint8_t & wholeBundleVec)> WholeBundleReadyCallback_t;
void SetCallbackFunction(const WholeBundleReadyCallback_t& wholeBundleReadyCallback);

class GStreamerAppSrcOutduct
{
public:
    GStreamerAppSrcOutduct(std::string shmSocketPath);
    ~GStreamerAppSrcOutduct();

    int PushRtpPacketToGStreamer(padded_vector_uint8_t& rtpPacketToTake);
   
    
    // Getters
    GstElement * GetAppSrc();
    GstElement * GetPipeline();
    
    // <private> 
    bool TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout);
    boost::circular_buffer<padded_vector_uint8_t> m_incomingRtpPacketQueue; // consider making this a pre allocated vector
    // thread members
    boost::mutex m_incomingQueueMutex;     
    boost::condition_variable m_incomingQueueCv;

    uint64_t m_numSamples = 1;

private:
    bool GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout);
    
    
    std::unique_ptr<boost::thread> m_processingThread;
    std::unique_ptr<boost::thread> m_busMonitoringThread;

    std::string m_shmSocketPath;
    bool m_running;
    void OnBusMessages();
    void PushData();

    // gst members
    GstBus *m_bus;
    GstMessage *m_gstMsg;
    GstStateChangeReturn m_GstStateChangeReturn;

    // setup functions
    int CreateElements();
    int BuildPipeline();
    int StartPlaying();
    GstFlowReturn PushDataCallback(GstElement *sink);
    
    GstElement *m_capsfilter;

    // pipeline members
    GstElement *m_pipeline;
    GstElement *m_appsrc;
    /* cap goes here*/
    GstElement *m_rtpjitterbuffer;
    GstElement *m_rtph264depay;
    GstElement *m_h264parse;
    GstElement *m_tee;
    GstElement *m_shmsink; 
    GstElement *m_shmQueue;
    GstElement *m_displayQueue;
    GstElement *m_decodebin;
    GstElement *m_videoconvert;
    GstElement *m_autovideosink;
    
    GstElement *m_fakesink;
    GstElement *m_appsink;
    GstElement *m_identity;
    GMainLoop *m_main_loop;
    // stat keeping 
    uint64_t m_totalIncomingCbOverruns = 0;

};





void SetGStreamerAppSrcOutductInstance(GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);



// Use sync=true if:
    // There is a human watching the output, e.g. movie playback
// Use sync=false if:
    // You are using a live source
    // The pipeline is being post-processed, e.g. neural net