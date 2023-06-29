
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

#define MAX_NUM_BUFFERS_QUEUE (10000)
#define MAX_SIZE_BYTES_QUEUE (0) // 0 = disable
#define MAX_SIZE_TIME_QUEUE (0) // 0 = disable

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
    GstElement * GetShmSink();
    GstElement * GetMp4Mux();

    // <private> 
    bool TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout);
    boost::circular_buffer<padded_vector_uint8_t> m_incomingRtpPacketQueue; // consider making this a pre allocated vector
    // thread members
    boost::mutex m_incomingQueueMutex;     
    boost::condition_variable m_incomingQueueCv;

    uint64_t m_numSamples = 0;

private:
    bool GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout);
    
    
    std::unique_ptr<boost::thread> m_processingThread;
    std::unique_ptr<boost::thread> m_busMonitoringThread;

    std::string m_shmSocketPath;
    volatile bool m_running;
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
    int CheckInitializationSuccess();
    
    GstElement *m_capsfilter;

    // pipeline members
    GstElement *m_pipeline;
    GstElement *m_appsrc;
    /* cap goes here*/

    GstElement *m_queue;
    GstElement *m_tee;
    
    GstElement *m_displayQueue;
    GstElement *m_rtpjitterbuffer;
    GstElement *m_rtph264depay;
    GstElement *m_h264parse;
    GstElement *m_decodebin;
    GstElement *m_shmsink; 

    GstElement *m_recordQueue;
    GstElement *m_record_rtpjitterbuffer;
    GstElement *m_record_rtph264depay;
    GstElement *m_record_h264parse;
    GstElement *m_record_mp4mux;
    GstElement *m_filesink;

    // GstElement *m_appsink;

    // stat keeping 
    uint64_t m_totalIncomingCbOverruns = 0;

};





void SetGStreamerAppSrcOutductInstance(GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);



// Use sync=true if:
    // There is a human watching the output, e.g. movie playback
// Use sync=false if:
    // You are using a live source
    // The pipeline is being post-processed, e.g. neural net

//     alignment=au means that each output buffer contains the NALs for a whole
// picture. alignment=nal just means that each output buffer contains
// complete NALs, but those do not need to represent a whole frame.