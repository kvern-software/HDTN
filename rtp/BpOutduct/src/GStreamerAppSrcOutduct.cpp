#include "GStreamerAppSrcOutduct.h"

#include "Logger.h"
#include "ThreadNamer.h"
// #include <boost/smart_ptr>

static void StartFeed(GstElement *source, guint size, GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);
static void StopFeed(GstElement *source, GStreamerAppSrcOutduct * gStreamerAppSrcOutduct);
// static bool PushData(uint64_t *unusedGstreamerPtr);
static guint g_sourceid = 0; 

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;


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


void OnBusMessages(GstBus *bus, GstMessage *msg, GStreamerAppSrcOutduct * gStreamerAppSrcOutduct) {
switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR: {
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
        LOG_INFO(subprocess) << "Got GST_MESSAGE_EOS (end of stream)";
        gst_element_set_state (s_gStreamerAppSrcOutduct->GetPipeline(), GST_STATE_NULL);
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


/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
static void StartFeed(GstElement *source, guint size, GStreamerAppSrcOutduct *gStreamerAppSrcOutduct)
{
    // if (g_sourceid == 0) {
        // LOG_DEBUG(subprocess) << "StartFeed";
    // g_print ("Start feeding\n");
    // g_sourceid = g_idle_add((GSourceFunc) PushData, NULL);
    // PushData(NULL);
//   }
}

static void StopFeed(GstElement *source, GStreamerAppSrcOutduct *gStreamerAppSrcOutduct)
{
    // if (g_sourceid != 0) {
    LOG_DEBUG(subprocess) << "StopFeed";
        // g_source_remove(g_sourceid);
        // g_sourceid = 0;
    // }
}

void GStreamerAppSrcOutduct::PushData()
{
    GstBuffer *buffer;
    GstFlowReturn ret;
    GstMapInfo map;
    
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    static GstClockTime duration = 33333333;

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

            /* Set its timestamp and duration */
            GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (m_numSamples, GST_SECOND, SAMPLE_RATE);
            GST_BUFFER_DURATION (buffer) = duration;

            // copy in from our local queue
            gst_buffer_map (buffer, &map, GST_MAP_WRITE);
            memcpy(map.data,incomingRtpFrame.data(), incomingRtpFrame.size());

            /* Push the buffer into the appsrc */
            gst_buffer_unmap (buffer, &map);
            ret = gst_app_src_push_buffer((GstAppSrc *) GetAppSrc(), buffer); // takes ownership of buffer
    
            m_numSamples += 1;

            if (ret != GST_FLOW_OK) {
                /* We got some error, stop sending data */
                LOG_ERROR(subprocess) << "Could not push data into app src";
                return;
            }
        }   
    }

    LOG_INFO(subprocess) << "returning out of push data";
    return;
}


GStreamerAppSrcOutduct::GStreamerAppSrcOutduct(std::string fileToSave) : m_fileToSave(fileToSave), m_running(true)
{
    m_incomingRtpPacketQueue.set_capacity(DEFAULT_NUM_CIRC_BUFFERS);
    // Initialize gstreamer first
    gst_init(NULL, NULL);

    m_processingThread = boost::make_unique<boost::thread>(boost::bind(&GStreamerAppSrcOutduct::PushData, this)); 

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

    gst_element_send_event(m_appsrc, gst_event_new_eos());
    m_running = false;
    m_busMonitoringThread->join();
}


int GStreamerAppSrcOutduct::CreateElements()
{
    m_appsrc = gst_element_factory_make("appsrc", NULL);
    m_rtpjitterbuffer = gst_element_factory_make("rtpjitterbuffer", NULL);
    m_rtph264depay = gst_element_factory_make("rtph264depay", NULL);
    m_h264parse = gst_element_factory_make("h264parse", NULL);
    m_queue = gst_element_factory_make("queue", NULL);
    m_mp4mux = gst_element_factory_make("mp4mux", NULL);
    m_filesink = gst_element_factory_make("filesink", NULL);

    m_pipeline   =  gst_pipeline_new(NULL);

    m_capsfilter = gst_element_factory_make ("capsfilter", NULL);


    g_object_set(G_OBJECT(m_filesink), "location", m_fileToSave.c_str(), NULL);
    g_object_set (G_OBJECT(m_appsrc), "format", GST_FORMAT_TIME, NULL);

    if (!m_appsrc || !m_rtpjitterbuffer || !m_rtph264depay || !m_h264parse || !m_queue || !m_mp4mux || !m_filesink )
    {
        LOG_ERROR(subprocess) << "Could not create all elements";
    }
    GstCaps * caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96");

    if (!caps) {
        LOG_ERROR(subprocess) << "Could not create caps";
    }
    
    g_object_set(G_OBJECT(m_appsrc), "caps", caps, NULL);

    GstCaps * filterCaps = gst_caps_from_string("video/quicktime, variant=(string)iso");
    g_object_set (m_capsfilter, "caps", filterCaps, NULL);

    // connect signals to our app
    g_signal_connect (m_appsrc, "need-data", G_CALLBACK (StartFeed), &g_sourceid);
    g_signal_connect (m_appsrc, "enough-data", G_CALLBACK (StopFeed), &g_sourceid);

    /* Register callback function to be notified of bus messages */
    m_bus = gst_element_get_bus(m_pipeline);

    return 0;
}


int GStreamerAppSrcOutduct::BuildPipeline()
{
    LOG_INFO(subprocess) << "Building Pipeline";
    
    gst_bin_add_many(GST_BIN(m_pipeline), m_appsrc, m_rtpjitterbuffer, m_rtph264depay, m_h264parse, m_queue, m_capsfilter, m_mp4mux, m_filesink, NULL);
    
    if (gst_element_link_many(m_appsrc, m_rtpjitterbuffer, m_rtph264depay, m_h264parse, m_capsfilter, m_queue,  m_mp4mux, m_filesink, NULL) != TRUE) {
        LOG_ERROR(subprocess) << "Elements could not be linked";
        return -1;
    }

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
        // LOG_DEBUG(subprocess) << "Pushing frame into incoming queue";
        if (m_incomingRtpPacketQueue.full())
            m_totalIncomingCbOverruns++;

        m_incomingRtpPacketQueue.push_back(std::move(rtpPacketToTake));  // copy out bundle to circular buffer for sending

        // LOG_INFO(subprocess) << m_incomingRtpPacketQueue.size();
    }
    m_incomingQueueCv.notify_one();

    return 0;
}

/**
 * If return true, we have data
 * If return false, we do not have data
*/
bool GStreamerAppSrcOutduct::TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration& timeout) {
    if (m_incomingRtpPacketQueue.size() == 0) { // if empty, we wait
        return GetNextIncomingPacketTimeout(timeout);
    }
    return true; 
}


bool GStreamerAppSrcOutduct::GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_incomingQueueMutex);
    if ((m_incomingRtpPacketQueue.size() == 0)) {
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