#include "GStreamerAppSrcOutduct.h"

#include "Logger.h"
#include "ThreadNamer.h"

// static void cb_new_pad (GstElement *element, GstPad *pad, gpointer data);
static void StartFeed(GstElement *source, guint size, GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);
static void StopFeed(GstElement *source, GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);
static guint g_sourceid = 0; 

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;
static volatile bool blocked;

static GStreamerAppSrcOutduct * s_gStreamerAppSrcOutduct;
void SetGStreamerAppSrcOutductInstance(GStreamerAppSrcOutduct * gStreamerAppSrcOutduct)
{
    s_gStreamerAppSrcOutduct = gStreamerAppSrcOutduct;
}


static void pad_added_handler (GstElement *src, GstPad *new_pad, GStreamerAppSrcOutduct *data) {
    GstPad *sink_pad = gst_element_get_static_pad (s_gStreamerAppSrcOutduct->GetShmSink(), "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sink_pad)) {
        g_print ("We are already linked. Ignoring.\n");
    } else {
        if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
            g_print("err linking");
        }
        GST_DEBUG_BIN_TO_DOT_FILE((GstBin *) s_gStreamerAppSrcOutduct->GetPipeline(), GST_DEBUG_GRAPH_SHOW_ALL, "gst_finished_linking");
    }
}

/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
static void StartFeed(GstElement *source, guint size, GStreamerAppSrcOutduct *gStreamerAppSrcOutduct)
{
    // if (g_sourceid == 0) {
        // g_print ("Start feeding\n");
        // g_sourceid = g_idle_add ((GSourceFunc) PushData, NULL);
    // }
    // LOG_DEBUG(subprocess) << "StartFeed";
    // blocked = false;
}

static void StopFeed(GstElement *source, GStreamerAppSrcOutduct *gStreamerAppSrcOutduct)
{
    // if (g_sourceid != 0) {
        // g_print ("Stop feeding\n");
        // g_source_remove (g_sourceid);
        // g_sourceid = 0;
    // }
    // LOG_INFO(subprocess) << "StopFeed";
    // blocked = true;
}

void GStreamerAppSrcOutduct::PushData()
{
    GstBuffer *buffer;
    GstFlowReturn ret;
    GstMapInfo map;
    
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    static GstClockTime duration = 33333333; //.0333 seconds

    while (m_running) {
        bool notInWaitForNewBundlesState = TryWaitForIncomingDataAvailable(timeout);
        if (notInWaitForNewBundlesState) {
            m_incomingQueueMutex.lock();
            padded_vector_uint8_t incomingRtpFrame(std::move(m_incomingRtpPacketQueue.front()));
            m_incomingRtpPacketQueue.pop_front();
            m_incomingQueueMutex.unlock();

            /* Create a new empty buffer */
            buffer = gst_buffer_new_and_alloc(incomingRtpFrame.size());

            /* copy in from our local queue */
            gst_buffer_map(buffer, &map, GST_MAP_WRITE);
            memcpy(map.data, incomingRtpFrame.data(), incomingRtpFrame.size());
         
            /* Set its timestamp and duration */
            GST_BUFFER_PTS(buffer) = gst_util_uint64_scale(m_numSamples, GST_SECOND, SAMPLE_RATE);
            GST_BUFFER_DURATION(buffer) = duration;

            /* Push the buffer into the appsrc */
            gst_buffer_unmap(buffer, &map);

            // code crashes if we get to push_buffer too quickly (too often?). Need to find a better solution. Crashes reguardless of the method used to push_buffer
            // boost::this_thread::sleep_for(boost::chrono::microseconds(50));
            ret = gst_app_src_push_buffer((GstAppSrc *) m_appsrc, buffer); // takes ownership of buffer we DO NOT deref

            if (ret != GST_FLOW_OK) { 
                GST_DEBUG_BIN_TO_DOT_FILE((GstBin *) m_pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "gst_error");
                /* We got some error */
                std::cout << "WARNING: could not push data into app src. Err code: " << ret << std::endl;
                continue;
            } 
            m_numSamples += 1;

        }   

        static guint buffers_in_queue;
        static guint buffers_in_appsrc_queue;

        g_object_get(G_OBJECT(m_appsrc), "current-level-buffers", &buffers_in_appsrc_queue, NULL);
        g_object_get(G_OBJECT(m_shmQueue), "current-level-buffers", &buffers_in_queue, NULL);

        // if ((m_numSamples % 5) == 0) {
        //     printf("buffers_in_queue:%u\n", buffers_in_queue);
        //     printf("buffers_in_appsrc_queue:%u\n", buffers_in_appsrc_queue);
        // }
    }

    LOG_INFO(subprocess) << "Exiting PushData processing thread";

    return; // get here only when shutting down
}



GStreamerAppSrcOutduct::GStreamerAppSrcOutduct(std::string shmSocketPath) : m_shmSocketPath(shmSocketPath), m_running(true)
{
    m_incomingRtpPacketQueue.set_capacity(DEFAULT_NUM_CIRC_BUFFERS);
    
    gst_init(NULL, NULL); // Initialize gstreamer first


    LOG_INFO(subprocess) << "Creating GStreamer appsrc pipeline. ShmSocketPath=" << m_shmSocketPath;
    CreateElements();
    BuildPipeline();

    m_busMonitoringThread = boost::make_unique<boost::thread>(
        boost::bind(&GStreamerAppSrcOutduct::OnBusMessages, this)); 

    StartPlaying();
    
    m_processingThread = boost::make_unique<boost::thread>(boost::bind(&GStreamerAppSrcOutduct::PushData, this)); 
}

GStreamerAppSrcOutduct::~GStreamerAppSrcOutduct()
{
    LOG_INFO(subprocess) << "Calling GStreamerAppSrcOutduct deconstructor";
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_totalIncomingCbOverruns: " << m_totalIncomingCbOverruns;
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_numSamples " << m_numSamples;
    
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    m_running = false;
    m_busMonitoringThread->join();
    m_processingThread->join();
}


int GStreamerAppSrcOutduct::CreateElements()
{
    m_appsrc = gst_element_factory_make("appsrc", NULL);
    m_rtpjitterbuffer = gst_element_factory_make("rtpjitterbuffer", NULL);
    m_rtph264depay = gst_element_factory_make("rtph264depay", NULL);
    m_h264parse = gst_element_factory_make("h264parse", NULL);
    m_tee = gst_element_factory_make("tee", NULL);
    
    m_shmQueue = gst_element_factory_make("queue", NULL);
    m_displayQueue = gst_element_factory_make("queue", NULL);
    
    m_shmsink = gst_element_factory_make("shmsink", NULL);
    
    m_decodebin = gst_element_factory_make("decodebin", NULL);
    m_videoconvert = gst_element_factory_make("videoconvert", NULL);

    m_pipeline   =  gst_pipeline_new(NULL);

    if (!m_appsrc || !m_rtpjitterbuffer || !m_rtph264depay || !m_h264parse \
        || !m_tee || !m_shmQueue || !m_displayQueue || !m_shmsink|| !m_decodebin \
        || !m_videoconvert || !m_autovideosink) {
        LOG_ERROR(subprocess) << "Could not create all elements";
        return -1;
    }
   
    g_object_set(G_OBJECT(m_shmQueue), "max-size-buffers", MAX_NUM_BUFFERS_QUEUE2, "max-size-bytes", MAX_SIZE_BYTES_QUEUE2, "max-size-time", MAX_SIZE_TIME_QUEUE2, NULL );
    g_object_set(G_OBJECT(m_shmsink), "socket-path", m_shmSocketPath.c_str(), "wait-for-connection", false, "sync", false, "async", false, "shm-size", SHMSINK_SIZE, NULL);
    g_object_set(G_OBJECT(m_fakesink), "async", false, "sync", false, "dump", false, NULL);

    /* set caps on the src element */
    GstCaps * caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96");
    g_object_set(G_OBJECT(m_appsrc), "emit-signals", true, "min-latency", 20000000, "is-live", true, "do-timestamp", true, "max-bytes", 20000000, "caps", caps, "format", GST_FORMAT_TIME, "block", true, NULL);
    gst_caps_unref(caps);

    // connect signals to our app
    g_signal_connect (m_appsrc, "need-data", G_CALLBACK (StartFeed), &g_sourceid);
    g_signal_connect (m_appsrc, "enough-data", G_CALLBACK (StopFeed), &g_sourceid);
    g_signal_connect (m_decodebin, "pad-added", G_CALLBACK (pad_added_handler), m_shmsink);

    /* Register callback function to be notified of bus messages */
    m_bus = gst_element_get_bus(m_pipeline);

    return 0;
}


int GStreamerAppSrcOutduct::BuildPipeline()
{
    LOG_INFO(subprocess) << "Building Pipeline";
    
    gst_bin_add_many(GST_BIN(m_pipeline), m_appsrc, m_shmQueue,  m_rtpjitterbuffer, m_rtph264depay, m_h264parse, m_decodebin, m_videoconvert, m_shmsink, NULL); // m_rtpjitterbuffer, m_rtph264depay, m_h264parse, m_tee, m_shmQueue, m_decodebin, m_videoconvert, m_autovideosink,  m_displayQueue
    
    if (gst_element_link_many(m_appsrc, m_shmQueue, NULL) != true) {
        LOG_ERROR(subprocess) << "Appsrc and queue could not be linked";
        return -1;
    }

    if (gst_element_link_many(m_shmQueue, m_rtpjitterbuffer, m_rtph264depay, m_h264parse, NULL) != true) {
        LOG_ERROR(subprocess) << "Queue and rtp pipeline could not be linked";
        return -1;
    }

    if (gst_element_link_many(m_h264parse, m_decodebin, NULL) !=  true) {
        LOG_ERROR(subprocess) << "Could not link elements to shmsink";
        return -1;
    }

    if (gst_element_link_many(m_decodebin, m_shmsink, NULL) != true) {
        LOG_ERROR(subprocess) << "Could not decodebin to sink";
        return -1;
    }

    // if (gst_element_link_many(m_tee, m_displayQueue, m_decodebin, NULL) != true) {
    //     LOG_ERROR(subprocess) << "Could not link elements to display";
    //     return -1;
    // }

    // if (gst_element_link_many(m_videoconvert, m_autovideosink, NULL) != true) {
    //     LOG_ERROR(subprocess) << "Could not link videoconvert to autovideosink";
    //     return -1;
    // }

    // if (gst_element_link_many(m_decodebin, m_autovideosink, NULL) != true) {
        // LOG_ERROR(subprocess) << "Could not link decode to autovideosink";
        // return -1;
    // }

    // decodebin requires dynamic linking after getting caps
    // g_signal_connect(m_decodebin, "pad-added", G_CALLBACK (cb_new_pad), m_autovideosink);


    // /**
    //  * Elements can not be linked until their pads are created. Pads on the qtdemux are not created until the video 
    //  * source provides "enough information" to the plugin that it can determine the type of media. We hook up a callback
    //  * function here to link the two halves of the pipeline together when the pad is added
    // */
    // g_signal_connect(m_h264parse , "pad-added", G_CALLBACK(OnPadAdded), m_mp4mux);


    LOG_INFO(subprocess) << "Succesfully built pipeline";
    return 0;
}

int GStreamerAppSrcOutduct::StartPlaying()
{
    /* Start playing the pipeline */
    gst_element_set_state (m_pipeline, GST_STATE_PLAYING);
    LOG_INFO(subprocess) << "Receiving bin launched";
    GST_DEBUG_BIN_TO_DOT_FILE((GstBin *) m_pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "gst_outduct");
    return 0;
}


void GStreamerAppSrcOutduct::OnBusMessages()
{
    return;
    while (m_running) 
    {
        // LOG_DEBUG(subprocess) << "Waiting for bus messages";
        GstMessage * msg = gst_bus_timed_pop(m_bus, GST_MSECOND*100);
        if (!msg) 
            continue;
        switch (GST_MESSAGE_TYPE (msg)) 
        {
            case GST_MESSAGE_ERROR: 
            {
                GError *err;
                gchar *debug;

                gst_message_parse_error (msg, &err, &debug);
                LOG_INFO(subprocess) << "Error:" << err->message;
                g_error_free (err);
                g_free (debug);
                break;
            }
            case GST_MESSAGE_EOS:
                /* end-of-stream */
                LOG_INFO(subprocess) << "Got GST_MESSAGE_EOS";
                m_running = false;
                // gst_element_set_state (m_pipeline, GST_STATE_NULL);
                break;
            case GST_MESSAGE_BUFFERING: 
                break;
            case GST_MESSAGE_TAG:
                LOG_INFO(subprocess) << "Got tag message";
                break;
            case GST_MESSAGE_ASYNC_DONE:
                LOG_INFO(subprocess) << "Got GST_MESSAGE_ASYNC_DONE";
                break;
            case GST_MESSAGE_STATE_CHANGED:
                LOG_INFO(subprocess) << "Got GST_MESSAGE_STATE_CHANGED";
                break;
            case GST_MESSAGE_CLOCK_LOST:
                break;
            default:
                /* Unhandled message */
                break;
        }
    }
}

int GStreamerAppSrcOutduct::PushRtpPacketToGStreamer(padded_vector_uint8_t &rtpPacketToTake)
{
    {
        boost::mutex::scoped_lock lock(m_incomingQueueMutex);// lock mutex 
        if (m_incomingRtpPacketQueue.full())
            m_totalIncomingCbOverruns++;

        m_incomingRtpPacketQueue.push_back(std::move(rtpPacketToTake));  // copy out bundle to circular buffer for sending
    }
    m_incomingQueueCv.notify_one();

    return 0;
}

/**
 * If return true, we have data
 * If return false, we do not have data
*/
bool GStreamerAppSrcOutduct::TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout) {
    if (m_incomingRtpPacketQueue.size() == 0 || (blocked == true)) { // if empty, we wait
        return GetNextIncomingPacketTimeout(timeout);
    }
    return true; 
}


bool GStreamerAppSrcOutduct::GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_incomingQueueMutex);
    if ((m_incomingRtpPacketQueue.size() == 0) || (blocked == true)) {
        m_incomingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }
    
    return true;
}




GstElement *GStreamerAppSrcOutduct::GetAppSrc()
{
    return m_appsrc;
}


GstElement * GStreamerAppSrcOutduct::GetPipeline()
{
    return m_pipeline;
}

GstElement *GStreamerAppSrcOutduct::GetShmSink()
{
    return m_shmsink;
}

GstElement * GStreamerAppSrcOutduct::GetVideoConv()
{
    return m_videoconvert;
}
