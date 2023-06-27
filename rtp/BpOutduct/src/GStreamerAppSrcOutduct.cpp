#include "GStreamerAppSrcOutduct.h"

#include "Logger.h"
#include "ThreadNamer.h"

// static void cb_new_pad (GstElement *element, GstPad *pad, gpointer data);
static void StartFeed(GstElement *source, guint size, GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);
static void StopFeed(GstElement *source, GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);
static guint g_sourceid = 0; 

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;
static volatile bool blocked;
gboolean PushData(uint8_t * not_used);

static GStreamerAppSrcOutduct * s_gStreamerAppSrcOutduct;
void SetGStreamerAppSrcOutductInstance(GStreamerAppSrcOutduct * gStreamerAppSrcOutduct)
{
    s_gStreamerAppSrcOutduct = gStreamerAppSrcOutduct;
}

void OnPadAdded(GstElement *element, GstPad *pad, GStreamerAppSrcOutduct *gstreamerAppSrcOutduct)
{
    GstCaps *newPadCaps = NULL;
    GstStructure *newPadStruct = NULL;
    const gchar *newPadType = NULL;


    LOG_INFO(subprocess) << "Received new pad " << GST_PAD_NAME (pad) << " from " << GST_ELEMENT_NAME (element);
    LOG_INFO(subprocess) << "Attempting to link pads";
    GstPad *sinkpad = gst_element_get_static_pad((GstElement *) gstreamerAppSrcOutduct, "sink");;
    if (gst_pad_is_linked (sinkpad)) {
        LOG_INFO(subprocess) << "We are already linked. Ignoring.";
        return;
    }

      /* Check the new pad's type */
    newPadCaps = gst_pad_get_current_caps (pad);
    newPadStruct = gst_caps_get_structure (newPadCaps, 0);
    newPadType = gst_structure_get_name (newPadStruct);
    LOG_INFO(subprocess) << newPadType;
    if (!g_str_has_prefix (newPadType, "video/x-h")) {
        LOG_INFO(subprocess) << "It has type " << newPadType << "which is not h2XX video. Ignoring." ;
        gst_object_unref(sinkpad);
        return;
    }

    /* We can now link this pad with the its corresponding sink pad */
    LOG_INFO(subprocess) << "Dynamic pad created, linking qtdemuxer/h264parser";
    int ret = gst_pad_link (pad, sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
        LOG_INFO(subprocess) << "Type is " << newPadType << " but link failed";
    } else {
        LOG_INFO(subprocess) << "Link succeeded (type " << newPadType << ")";
    }
    gst_object_unref(sinkpad);
}


/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
static void StartFeed(GstElement *source, guint size, GStreamerAppSrcOutduct *gStreamerAppSrcOutduct)
{
    // if (g_sourceid == 0) {
        g_print ("Start feeding\n");
        // g_sourceid = g_idle_add ((GSourceFunc) PushData, NULL);
    // }
    // LOG_DEBUG(subprocess) << "StartFeed";
    // blocked = false;
}

static void StopFeed(GstElement *source, GStreamerAppSrcOutduct *gStreamerAppSrcOutduct)
{
    // if (g_sourceid != 0) {
        g_print ("Stop feeding\n");
        // g_source_remove (g_sourceid);
        // g_sourceid = 0;
    // }
    // LOG_DEBUG(subprocess) << "StopFeed";
    // blocked = true;
}

gboolean PushData(uint8_t * not_used)
{
    std::cout << "in push data" << std::endl;
    GstBuffer *buffer;
    GstFlowReturn ret;
    GstMapInfo map;
    
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    static GstClockTime duration = 3333333333;

    bool notInWaitForNewBundlesState = s_gStreamerAppSrcOutduct->TryWaitForIncomingDataAvailable(timeout);
    if (notInWaitForNewBundlesState) {
        LOG_DEBUG(subprocess) << "Processing front of incoming queue"; 
        s_gStreamerAppSrcOutduct->m_incomingQueueMutex.lock();
        padded_vector_uint8_t incomingRtpFrame(std::move(s_gStreamerAppSrcOutduct->m_incomingRtpPacketQueue.front()));
        s_gStreamerAppSrcOutduct->m_incomingRtpPacketQueue.pop_front();
        s_gStreamerAppSrcOutduct->m_incomingQueueMutex.unlock();

        // rtp_frame * frame = (rtp_frame *) incomingRtpFrame.data();
        // frame->print_header();
        
        /* Create a new empty buffer */
        buffer = gst_buffer_new_and_alloc(incomingRtpFrame.size());

        /* Set its timestamp and duration */
        GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (s_gStreamerAppSrcOutduct->m_numSamples, GST_SECOND, SAMPLE_RATE);
        GST_BUFFER_DURATION (buffer) = duration;

        // copy in from our local queue
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);
        memcpy(map.data, incomingRtpFrame.data(), incomingRtpFrame.size());

        /* Push the buffer into the appsrc */
        gst_buffer_unmap(buffer, &map);
        ret = gst_app_src_push_buffer((GstAppSrc *) s_gStreamerAppSrcOutduct->GetAppSrc(), buffer); // takes ownership of buffer

        if (ret != GST_FLOW_OK) { 
            /* We got some error, stop sending data */
            LOG_ERROR(subprocess) << "Could not push data into app src";
            return FALSE;
        } 
        s_gStreamerAppSrcOutduct->m_numSamples += 1;

        return TRUE;
    }   

    return FALSE;
}

void GStreamerAppSrcOutduct::PushData()
{

    // m_main_loop = g_main_loop_new (NULL, FALSE);
    // g_main_loop_run (m_main_loop);

    GstBuffer *buffer;
    GstFlowReturn ret;
    GstMapInfo map;
    
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    static GstClockTime duration = 33333333333333;

    while (1) 
    {
        bool notInWaitForNewBundlesState = TryWaitForIncomingDataAvailable(timeout);
        if (notInWaitForNewBundlesState) {
            // LOG_DEBUG(subprocess) << "Processing front of incoming queue"; 
            m_incomingQueueMutex.lock();
            padded_vector_uint8_t incomingRtpFrame(std::move(m_incomingRtpPacketQueue.front()));
            m_incomingRtpPacketQueue.pop_front();
            m_incomingQueueMutex.unlock();

            // rtp_frame * frame = (rtp_frame *) incomingRtpFrame.data();
            // frame->print_header();
            
            /* Create a new empty buffer */
            buffer = gst_buffer_new_and_alloc(incomingRtpFrame.size());

            // copy in from our local queue
            gst_buffer_map(buffer, &map, GST_MAP_WRITE);
            memcpy(map.data, incomingRtpFrame.data(), incomingRtpFrame.size());
         
            /* Set its timestamp and duration */
            GST_BUFFER_PTS(buffer) = gst_util_uint64_scale(m_numSamples, GST_SECOND, SAMPLE_RATE);
            GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(m_numSamples, GST_SECOND, SAMPLE_RATE);

        //   GST_DEBUG_BIN_TO_DOT_FILE((GstBin *) m_pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "gst_playing");

            /* Push the buffer into the appsrc */
            gst_buffer_unmap(buffer, &map);
            ret = gst_app_src_push_buffer((GstAppSrc *) GetAppSrc(), buffer); // takes ownership of buffer
            // g_signal_emit_by_name((GstAppSrc *) GetAppSrc(), "push-buffer", buffer, &ret); // does not take ownership of buffer you must deref

            if (ret != GST_FLOW_OK) { 
                /* We got some error, stop sending data */
                LOG_ERROR(subprocess) << "Could not push data into app src";
                return;
            } 
            m_numSamples += 1;

        }   

        // guint buffers_in_queue;
        // g_object_get(G_OBJECT(m_shmQueue), "current-level-buffers", &buffers_in_queue);
        // std::cout << "buffers_in_queue: " << buffers_in_queue << std::endl;
    }

    LOG_INFO(subprocess) << "returning out of push data";
    return;
}

// GstFlowReturn GStreamerAppSrcOutduct::
GstFlowReturn NewSample(GstElement *sink)
{
    GstSample *sample;
    /* Retrieve the buffer */
    g_print ("* new sample callback *\n");

    g_signal_emit_by_name (sink, "pull-sample", &sample);
    if (sample) {
        /* The only thing we do in this example is print a * to indicate a received buffer */
        g_print ("* got buffer *\n");
        gst_sample_unref (sample);
        return GST_FLOW_OK;
    }
    return GST_FLOW_ERROR;
}

GStreamerAppSrcOutduct::GStreamerAppSrcOutduct(std::string shmSocketPath) : m_shmSocketPath(shmSocketPath), m_running(true)
{
    m_incomingRtpPacketQueue.set_capacity(DEFAULT_NUM_CIRC_BUFFERS);
    
    gst_init(NULL, NULL); // Initialize gstreamer first

    m_processingThread = boost::make_unique<boost::thread>(boost::bind(&GStreamerAppSrcOutduct::PushData, this)); 

    LOG_INFO(subprocess) << "Creating GStreamer appsrc pipeline. ShmSocketPath=" << m_shmSocketPath;
    CreateElements();
    BuildPipeline();

    m_busMonitoringThread = boost::make_unique<boost::thread>(
        boost::bind(&GStreamerAppSrcOutduct::OnBusMessages, this)); 

    StartPlaying();
}

GStreamerAppSrcOutduct::~GStreamerAppSrcOutduct()
{
    LOG_INFO(subprocess) << "Calling GStreamerAppSrcOutduct deconstructor";
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_totalIncomingCbOverruns: " << m_totalIncomingCbOverruns;
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_numSamples " << m_numSamples;
    gst_element_send_event(m_appsrc, gst_event_new_eos());
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
    m_autovideosink = gst_element_factory_make("glimagesink", NULL);

    m_fakesink = gst_element_factory_make("fakesink", NULL);
    m_appsink = gst_element_factory_make("appsink", NULL);
    m_identity = gst_element_factory_make("identity", NULL);

    m_pipeline   =  gst_pipeline_new(NULL);

    if (!m_appsrc || !m_rtpjitterbuffer || !m_rtph264depay || !m_h264parse \
        || !m_tee || !m_shmQueue || !m_displayQueue || !m_shmsink|| !m_decodebin \
        || !m_videoconvert || !m_autovideosink) {
        LOG_ERROR(subprocess) << "Could not create all elements";
        return -1;
    }
   
            //  "max-latency", -1,  "is-live", true, "do-timestamp", true,  "min-latency", -1,  "format", GST_FORMAT_TIME, 
    g_object_set(G_OBJECT(m_shmsink), "socket-path", m_shmSocketPath.c_str(), "wait-for-connection", false, "sync", false, "async", false, NULL);
    g_object_set(G_OBJECT(m_fakesink), "async", false, "sync", false, "dump", true, NULL);
    // g_object_set(G_OBJECT(m_autovideosink), "async-handling", true, NULL);

    /* set caps on the src element */
    GstCaps * caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96");
    g_object_set(G_OBJECT(m_appsrc), "async", false, "sync", false, "min-latency", 20000000, "is-live", true, "do-timestamp", true, "max-bytes", 20000, "caps", caps, "format", GST_FORMAT_TIME, "block", true, NULL);
    g_object_set (G_OBJECT(m_appsink), "async", false, "sync", false, "emit-signals", true, "caps", caps, NULL);
    gst_caps_unref(caps);

    // connect signals to our app
    g_signal_connect (m_appsrc, "need-data", G_CALLBACK (StartFeed), &g_sourceid);
    g_signal_connect (m_appsrc, "enough-data", G_CALLBACK (StopFeed), &g_sourceid);
    g_signal_connect(m_appsink, "new-sample", G_CALLBACK (NewSample), 0);

    /* Register callback function to be notified of bus messages */
    m_bus = gst_element_get_bus(m_pipeline);



    return 0;
}


int GStreamerAppSrcOutduct::BuildPipeline()
{
    LOG_INFO(subprocess) << "Building Pipeline";
    
    gst_bin_add_many(GST_BIN(m_pipeline), m_appsrc, m_shmQueue, m_shmsink, m_fakesink, m_identity,  m_appsink, NULL); // m_rtpjitterbuffer, m_rtph264depay, m_h264parse, m_tee, m_shmQueue, m_decodebin, m_videoconvert, m_autovideosink,  m_displayQueue
    
    if (gst_element_link_many(m_appsrc, m_shmQueue, m_shmsink, NULL) != true) {
        LOG_ERROR(subprocess) << "Elements could not be linked";
        return -1;
    }

    // if (gst_element_link_many(m_appsrc, m_rtpjitterbuffer, m_rtph264depay, m_h264parse, m_tee, NULL) != true) {
    //     LOG_ERROR(subprocess) << "Elements could not be linked to tee";
    //     return -1;
    // }

    // if (gst_element_link_many(m_tee, m_shmQueue, m_shmsink, NULL) !=  true) {
    //     LOG_ERROR(subprocess) << "Could not link elements to shmsink";
    //     return -1;
    // }

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
    while (m_running) 
    {
        // LOG_DEBUG(subprocess) << "Waiting for bus messages";
        GstMessage * msg = gst_bus_timed_pop(m_bus, -1);
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
                gst_element_set_state (m_pipeline, GST_STATE_NULL);
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


// static void cb_new_pad (GstElement *element, GstPad *pad, gpointer data)
// {
// g_print("CALLBACK NEW PAD\n\n\n\n\n");
//   gchar *name;
//   GstElement *other = (GstElement *) data;
//   name = gst_pad_get_name (pad);
//   g_print ("A new pad %s was created for %s\n", name, gst_element_get_name(element));
//   g_free (name);
// 
//   g_print ("element %s will be linked to %s\n",
        //    gst_element_get_name(element),
        //    gst_element_get_name(other));
//   gst_element_link(element, other);
// 
    // GST_DEBUG_BIN_TO_DOT_FILE((GstBin *) s_gStreamerAppSrcOutduct->GetPipeline(), GST_DEBUG_GRAPH_SHOW_ALL, "gst_linked");
// 
// }