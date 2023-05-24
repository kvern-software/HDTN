
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
#define DEFAULT_NUM_CIRC_BUFFERS 500

typedef boost::function<void(padded_vector_uint8_t & wholeBundleVec)> WholeBundleReadyCallback_t;
void SetCallbackFunction(const WholeBundleReadyCallback_t& wholeBundleReadyCallback);





class GStreamerAppSrcOutduct
{
public:
    GStreamerAppSrcOutduct(std::string fileToSave);
    ~GStreamerAppSrcOutduct();

    int PushRtpPacketToGStreamer(padded_vector_uint8_t& rtpPacketToTake);
   
    
    // Getters
    GstElement * GetAppSrc();
    GstElement * GetPipeline();
    
    // <private> 
    boost::circular_buffer<padded_vector_uint8_t> m_incomingRtpPacketQueue; // consider making this a pre allocated vector
    // thread members
    boost::mutex m_incomingQueueMutex;     
    boost::condition_variable m_incomingQueueCv;
    bool TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout);
    bool GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout);

    uint64_t m_numSamples;



private:
    std::unique_ptr<boost::thread> m_processingThread;
    std::unique_ptr<boost::thread> m_busMonitoringThread;

    std::string m_fileToSave;
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

    GstElement *m_capsfilter;

    // pipeline members
    GstElement *m_pipeline;
    GstElement *m_appsrc;
    /* cap goes here*/
    GstElement *m_rtpjitterbuffer;
    GstElement *m_rtph264depay;
    GstElement *m_h264parse;
    GstElement *m_queue;
    GstElement *m_mp4mux;
    GstElement *m_filesink;
    
    GMainLoop *m_loop;
    
    // stat keeping 
    uint64_t m_totalIncomingCbOverruns = 0;

};




void SetGStreamerAppSrcOutductInstance(GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);
