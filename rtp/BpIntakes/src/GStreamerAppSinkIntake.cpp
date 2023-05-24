#include "GStreamerAppSinkIntake.h"


#include "Logger.h"
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

/**
 * I do not know if there is a better way to do this. The problem is complicated:
 * 1. We can not use member functions for the GStreamer callbacks since they are pure C and do not 
 *  support the 'this' keyword.
 * 2. Therefore we can not set a callback function the usual way in the constructor.
 * 3. The callback function can not be a member of the class since the Gstreamder callbacks 
 * are not member functions
*/
static WholeBundleReadyCallback_t s_wholeBundleReadyCallback;
void SetCallbackFunction(const WholeBundleReadyCallback_t& wholeBundleReadyCallback)
{
    s_wholeBundleReadyCallback = wholeBundleReadyCallback;
}

void OnPadAdded(GstElement *element, GstPad *pad, GStreamerAppSinkIntake *gStreamerAppSinkIntake)
{
    GstCaps *newPadCaps = NULL;
    GstStructure *newPadStruct = NULL;
    const gchar *newPadType = NULL;


    LOG_INFO(subprocess) << "Received new pad " << GST_PAD_NAME (pad) << " from " << GST_ELEMENT_NAME (element);
    LOG_INFO(subprocess) << "Attempting to link pads";
    GstPad *sinkpad = gst_element_get_static_pad((GstElement *) gStreamerAppSinkIntake, "sink");;
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

void OnNewSampleFromSink(GstElement *element, GStreamerAppSinkIntake *GStreamerAppSinkIntake)
{
    GstSample *sample, *copySample;
    GstBuffer *buffer;
    GstMapInfo map;

    /* get the sample from appsink */
    sample = gst_app_sink_pull_sample(GST_APP_SINK (element));
    /* make a copy */
    copySample = gst_sample_copy(sample);
    gst_sample_unref(sample);  
    buffer = gst_sample_get_buffer(copySample);
    if(!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      LOG_WARNING(subprocess) <<"could not map buffer";
      return;
    }

    // Copy for final time into buffer, from here we can std::move rather than copy
    padded_vector_uint8_t bufferToForward(map.size);
    memcpy(bufferToForward.data(), map.data, map.size);
    
    // the order here matters, unref before exporting bundle
    gst_sample_unref(copySample);  

    /* push buffer to HDTN */
    s_wholeBundleReadyCallback(bufferToForward);
}





GStreamerAppSinkIntake::GStreamerAppSinkIntake(std::string fileToStream) : 
    m_fileToStream(fileToStream), m_running(true)
{
    
    // Initialize gstreamer first
    gst_init(NULL, NULL);

    CreateElements();
    BuildPipeline();

    m_busMonitoringThread = boost::make_unique<boost::thread>(
        boost::bind(&GStreamerAppSinkIntake::OnBusMessages, this)); 

    StartPlaying();
}

GStreamerAppSinkIntake::~GStreamerAppSinkIntake()
{
    LOG_INFO(subprocess) << "Calling GStreamerAppSinkIntake deconstructor";   
    m_running = false;
    m_busMonitoringThread->join();
    gst_element_set_state(m_source, GST_STATE_NULL);
    gst_element_set_state(m_sink, GST_STATE_NULL);
}

void GStreamerAppSinkIntake::OnBusMessages()
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

int GStreamerAppSinkIntake::CreateElements()
{
    // m_loop = g_main_loop_new(NULL, FALSE);

    m_source         =  gst_element_factory_make("filesrc", NULL);
    m_qtdemux        =  gst_element_factory_make("qtdemux", NULL);
    m_h264parse      =  gst_element_factory_make("h264parse", NULL);
    m_rtph264pay     =  gst_element_factory_make("rtph264pay", NULL);
    // m_queue       =  gst_element_factory_make("queue", NULL);
    m_sink           =  gst_element_factory_make("appsink", NULL);
    m_progressreport =  gst_element_factory_make("progressreport", NULL);
    m_pipeline       =  gst_pipeline_new(NULL);

    g_object_set(G_OBJECT(m_source), "location", m_fileToStream.c_str(), NULL);
    g_object_set(G_OBJECT(m_progressreport), "update-freq", 1, NULL);

    return 0;
}

int GStreamerAppSinkIntake::BuildPipeline()
{
    LOG_INFO(subprocess) << "Building Pipeline";
    
    gst_bin_add_many(GST_BIN(m_pipeline), m_source,  m_qtdemux, m_h264parse, m_rtph264pay, m_progressreport, m_sink, NULL);
    
    if (gst_element_link(m_source, m_qtdemux) != TRUE) {
        LOG_ERROR(subprocess) << "Source and qtmux could not be linked";
        return -1;
    }

    if (gst_element_link_many(m_h264parse, m_rtph264pay, m_progressreport, m_sink, NULL) != TRUE) {
        LOG_ERROR(subprocess) << "Elements could not be linked";
        return -1;
    }

    /**
     * Elements can not be linked until their pads are created. Pads on the qtdemux are not created until the video 
     * source provides "enough information" to the plugin that it can determine the type of media. We hook up a callback
     * function here to link the two halves of the pipeline together when the pad is added
    */
    g_signal_connect(m_qtdemux, "pad-added", G_CALLBACK(OnPadAdded), m_h264parse);

    /**
     * Register callback function to be notified of bus messages
    */
    m_bus = gst_element_get_bus(m_pipeline);
    // gst_bus_add_signal_watch(m_bus);
    // g_signal_connect(m_bus, "message", G_CALLBACK(OnBusMessages), NULL);

    LOG_INFO(subprocess) << "Succesfully built pipeline";

    return 0;
}

int GStreamerAppSinkIntake::StartPlaying()
{
    m_GstStateChangeReturn = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    if (m_GstStateChangeReturn == GST_STATE_CHANGE_FAILURE) {
        LOG_ERROR(subprocess) << "Unable to set the pipeline to the playing state";
        return -1;
    }
    
    LOG_INFO(subprocess) << "Capture bin launched";


    
    /* we use appsink in push mode, it sends us a signal when data is available
    * and we pull out the data in the signal callback. Also, enable sync so the data is livestreamed 
    and not all sent in a single instant */
    g_object_set(G_OBJECT(m_sink), "emit-signals", TRUE, "sync", TRUE, NULL);
    g_signal_connect(m_sink, "new-sample", G_CALLBACK(OnNewSampleFromSink), NULL);
    

    LOG_INFO(subprocess) << "Play bin launched";   
    LOG_INFO(subprocess) << "Going to set state to play";
    /* launching things */
    gst_element_set_state(m_sink, GST_STATE_PLAYING);
    gst_element_set_state(m_source, GST_STATE_PLAYING);

    // g_main_loop_run(m_loop);

    return 0;
}
