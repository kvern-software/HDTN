#include "GStreamerAppSrcOutduct.h"

#include "Logger.h"
#include "ThreadNamer.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;
static volatile bool blocked;
static GstClockTime duration    = 33333333; // .0.016666667 seconds aka 60fps
static GStreamerAppSrcOutduct * s_gStreamerAppSrcOutduct;

void SetGStreamerAppSrcOutductInstance(GStreamerAppSrcOutduct * gStreamerAppSrcOutduct)
{
    s_gStreamerAppSrcOutduct = gStreamerAppSrcOutduct;
}


GStreamerAppSrcOutduct::GStreamerAppSrcOutduct(std::string shmSocketPath) : 
    m_shmSocketPath(shmSocketPath), m_running(true), m_runDisplayThread(true)
{
    m_incomingRtpPacketQueue.set_capacity(DEFAULT_NUM_CIRC_BUFFERS);
    m_incomingRtpPacketQueueForDisplay.set_capacity(DEFAULT_NUM_CIRC_BUFFERS);
    m_incomingRtpPacketQueueForFilesink.set_capacity(DEFAULT_NUM_CIRC_BUFFERS);


    boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    m_bundleCallbackAsyncListenerPtr       = boost::make_unique<AsyncListener<CbQueue_t>>(m_incomingRtpPacketQueue, timeout);
    m_rtpPacketToDisplayAsyncListenerPtr   = boost::make_unique<AsyncListener<CbQueue_t>>(m_incomingRtpPacketQueueForDisplay, timeout);
    m_rtpPacketToFilesinkAsyncListenerPtr  = boost::make_unique<AsyncListener<CbQueue_t>>(m_incomingRtpPacketQueueForFilesink, timeout);

    LOG_INFO(subprocess) << "Creating GStreamer appsrc pipeline. ShmSocketPath=" << m_shmSocketPath;
    
    gst_init(NULL, NULL); // Initialize gstreamer first
    
    if ((CreateElements() == 0) && (BuildPipeline() == 0)) {
        m_busMonitoringThread = boost::make_unique<boost::thread>(boost::bind(&GStreamerAppSrcOutduct::OnBusMessages, this)); 

        StartPlaying();
        
        m_packetTeeThread = boost::make_unique<boost::thread>(boost::bind(&GStreamerAppSrcOutduct::TeeDataToQueuesThread, this)); 
        m_displayThread =  boost::make_unique<boost::thread>(boost::bind(&GStreamerAppSrcOutduct::PushDataToDisplayThread, this)); 
        m_filesinkThread =  boost::make_unique<boost::thread>(boost::bind(&GStreamerAppSrcOutduct::PushDataToFilesinkThread, this)); 
    } else {
        LOG_ERROR(subprocess) << "Could not initialize GStreamerAppSrcOutduct. Aborting";
    }
}

GStreamerAppSrcOutduct::~GStreamerAppSrcOutduct()
{
    LOG_INFO(subprocess) << "Calling GStreamerAppSrcOutduct deconstructor";
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_totalIncomingCbOverruns: " << m_totalIncomingCbOverruns;
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_totalDisplayCbOverruns: " << m_totalDisplayCbOverruns;
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_totalFilesinkCbOverruns: " << m_totalFilesinkCbOverruns;
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_numDisplaySamples " << m_numDisplaySamples;
    LOG_INFO(subprocess) << "GStreamerAppSrcOutduct::m_numFilesinkSamples " << m_numFilesinkSamples;


    // GstStructure rtpstats;
    // g_object_get(G_OBJECT(m_rtpjitterbuffer), "stats", &rtpstats, NULL);
    // char * num_push =       g_strdup_value_contents(gst_structure_get_value(&rtpstats, "num-push"));
    // char * num_lost =       g_strdup_value_contents(gst_structure_get_value(&rtpstats, "num-lost"));
    // char * num_duplicates = g_strdup_value_contents(gst_structure_get_value(&rtpstats, "num-duplicates"));
    // char * avg_jitter =     g_strdup_value_contents(gst_structure_get_value(&rtpstats, "avg-jitter"));
    // LOG_INFO(subprocess) << "rtpjitterbuffer:num-push: " <<       num_push;
    // LOG_INFO(subprocess) << "rtpjitterbuffer:num-lost: " <<       num_lost;
    // LOG_INFO(subprocess) << "rtpjitterbuffer:num-duplicates: " << num_duplicates;
    // LOG_INFO(subprocess) << "rtpjitterbuffer:avg-jitter: " <<     avg_jitter;  
    
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    
    m_running = false;
    m_runDisplayThread = false;

    m_busMonitoringThread->join();
    m_packetTeeThread->join();
}

int GStreamerAppSrcOutduct::CreateElements()
{
    /* Display */
    m_displayAppsrc = gst_element_factory_make("appsrc", NULL);
    m_displayQueue = gst_element_factory_make("queue", NULL);
    m_rtpjitterbuffer = gst_element_factory_make("rtpjitterbuffer", NULL);
    m_rtph264depay = gst_element_factory_make("rtph264depay", NULL);
    m_h264parse = gst_element_factory_make("h264parse", NULL);
    m_h264timestamper = gst_element_factory_make("h264timestamper", NULL);
    m_decodeQueue = gst_element_factory_make("queue", NULL);
    m_avdec_h264 = gst_element_factory_make("avdec_h264", NULL);
    m_postDecodeQueue = gst_element_factory_make("queue", NULL);
    m_displayShmsink = gst_element_factory_make("shmsink", NULL);
   
    /* Filesink */
    m_filesinkAppsrc = gst_element_factory_make("appsrc", NULL);
    m_filesinkQueue = gst_element_factory_make("queue", NULL);
    m_filesinkShmsink = gst_element_factory_make("shmsink", NULL);
    
    m_pipeline   =  gst_pipeline_new(NULL);
   
    /* Configure queues */
    g_object_set(G_OBJECT(m_displayQueue), "max-size-buffers", MAX_NUM_BUFFERS_QUEUE, "max-size-bytes", MAX_SIZE_BYTES_QUEUE, "max-size-time", MAX_SIZE_TIME_QUEUE, "min-threshold-time", (uint64_t) 0,  NULL );
    g_object_set(G_OBJECT(m_decodeQueue), "max-size-buffers", 0, "max-size-bytes", 0, "max-size-time", 0, "min-threshold-time", (uint64_t) 0, "leaky", 0,  NULL );
    g_object_set(G_OBJECT(m_filesinkQueue), "max-size-buffers", 0, "max-size-bytes", MAX_SIZE_BYTES_QUEUE, "max-size-time", MAX_SIZE_TIME_QUEUE, "leaky", 0, NULL );


    /* Configure shared memory sinks */
    g_object_set(G_OBJECT(m_displayShmsink), "socket-path", m_shmSocketPath.c_str(), "wait-for-connection", false, "sync", false, "async", false, NULL);
    g_object_set(G_OBJECT(m_filesinkShmsink), "socket-path", "/tmp/hdtn_gst_shm_outduct_filesink", "wait-for-connection", false, "sync", false, "async", false, NULL);

    /* Configure rtpjitterbuffer */
    g_object_set(G_OBJECT(m_rtpjitterbuffer), "latency", RTP_LATENCY_MILLISEC, "max-dropout-time", RTP_MAX_DROPOUT_TIME_MILLISEC,
        "max-misorder-time", RTP_MAX_MISORDER_TIME_MIILISEC, "mode", RTP_MODE, "drop-on-latency", true, NULL);
    
    /* Configure decoder */
    g_object_set(G_OBJECT(m_avdec_h264), "lowres", 0, "output-corrupt", false, "discard-corrupted-frames", true,  NULL);

    /* set caps on the src element */
    GstCaps * caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96");
    g_object_set(G_OBJECT(m_displayAppsrc), "emit-signals", false, "min-latency", 0, "is-live", true, "do-timestamp", true, "max-bytes", GST_APPSRC_MAX_BYTES_IN_BUFFER, "caps", caps, "format", GST_FORMAT_TIME, "block", false, NULL);
    g_object_set(G_OBJECT(m_filesinkAppsrc), "emit-signals", false, "min-latency", 0, "is-live", false, "do-timestamp", false, "max-bytes", GST_APPSRC_MAX_BYTES_IN_BUFFER, "caps", caps, "format", GST_FORMAT_TIME, "block", false, NULL);
    gst_caps_unref(caps);

    /* Register our bus to be notified of bus messages */
    m_bus = gst_element_get_bus(m_pipeline);

    return 0;
}


int GStreamerAppSrcOutduct::BuildPipeline()
{
    LOG_INFO(subprocess) << "Building Pipeline";
    
    gst_bin_add_many(GST_BIN(m_pipeline), m_displayAppsrc, m_displayQueue, m_rtpjitterbuffer, m_rtph264depay, m_h264parse, m_h264timestamper, m_decodeQueue, m_avdec_h264, m_postDecodeQueue, m_displayShmsink, \
        m_filesinkAppsrc, m_filesinkQueue, m_filesinkShmsink, NULL);

    if (gst_element_link_many(m_displayAppsrc, m_displayQueue, m_rtpjitterbuffer, m_rtph264depay, m_h264parse, m_h264timestamper, m_decodeQueue, m_avdec_h264, m_postDecodeQueue, m_displayShmsink, NULL) != true) {
        LOG_ERROR(subprocess) << "Display pipeline could not be linked";
        return -1;
    }
        
    if (gst_element_link_many(m_filesinkAppsrc, m_filesinkQueue, m_filesinkShmsink, NULL) !=  true) {
        LOG_ERROR(subprocess) << "Filesink pipeline could not be linked";
        return -1;
    }

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


void GStreamerAppSrcOutduct::TeeDataToQueuesThread()
{
    padded_vector_uint8_t incomingRtpFrame;
    incomingRtpFrame.reserve(1400);

    while (1) {
        bool notInWaitForNewBundlesState = m_bundleCallbackAsyncListenerPtr->TryWaitForIncomingDataAvailable();
        if (notInWaitForNewBundlesState) {
            /* Make local copy to allow bundle thread to continue asap */
            m_bundleCallbackAsyncListenerPtr->Lock();
                incomingRtpFrame = std::move(m_bundleCallbackAsyncListenerPtr->m_queue.front()); // we now have a local copy, can unlock from incoming bundle queue
                m_bundleCallbackAsyncListenerPtr->m_queue.pop_front();
            m_bundleCallbackAsyncListenerPtr->Unlock();
            m_bundleCallbackAsyncListenerPtr->Notify();

            /* hard copy the data to the filesink queue */
            m_rtpPacketToFilesinkAsyncListenerPtr->Lock();
                padded_vector_uint8_t frameToPush(incomingRtpFrame.size());
                memcpy(frameToPush.data(), incomingRtpFrame.data(), incomingRtpFrame.size());
                if (m_rtpPacketToFilesinkAsyncListenerPtr->m_queue.full())
                    m_totalFilesinkCbOverruns++;
                m_rtpPacketToFilesinkAsyncListenerPtr->m_queue.push_front(std::move(frameToPush));
            m_rtpPacketToFilesinkAsyncListenerPtr->Unlock();
            m_rtpPacketToFilesinkAsyncListenerPtr->Notify();


            /* zero copy the data to the display queue */
            m_rtpPacketToDisplayAsyncListenerPtr->Lock();
                if (m_rtpPacketToDisplayAsyncListenerPtr->m_queue.full())
                    m_totalDisplayCbOverruns++;
                m_rtpPacketToDisplayAsyncListenerPtr->m_queue.push_front(std::move(incomingRtpFrame));

            m_rtpPacketToDisplayAsyncListenerPtr->Unlock();
            m_rtpPacketToDisplayAsyncListenerPtr->Notify();
        }
    }

}

void GStreamerAppSrcOutduct::PushDataToFilesinkThread()
{
    static HdtnGstHandoffUtils_t hdtnGstHandoffUtils;
    static padded_vector_uint8_t incomingRtpFrame;
    incomingRtpFrame.reserve(1400);

    while (m_running) {
        
        bool notInWaitForNewPacketsState = m_rtpPacketToFilesinkAsyncListenerPtr->TryWaitForIncomingDataAvailable();
        if (notInWaitForNewPacketsState) {
            m_rtpPacketToFilesinkAsyncListenerPtr->Lock();
                incomingRtpFrame = std::move(m_rtpPacketToFilesinkAsyncListenerPtr->m_queue.front());
                m_rtpPacketToFilesinkAsyncListenerPtr->PopFront();
            m_rtpPacketToFilesinkAsyncListenerPtr->Unlock();
            m_rtpPacketToFilesinkAsyncListenerPtr->Notify();


            hdtnGstHandoffUtils.buffer = gst_buffer_new_and_alloc(incomingRtpFrame.size());

            /* copy in from our local queue */
            gst_buffer_map(hdtnGstHandoffUtils.buffer, &hdtnGstHandoffUtils.map, GST_MAP_WRITE);
            memcpy(hdtnGstHandoffUtils.map.data, incomingRtpFrame.data(), incomingRtpFrame.size());
         
            /* Set its timestamp and duration */
            GST_BUFFER_PTS(hdtnGstHandoffUtils.buffer) = gst_util_uint64_scale(m_numFilesinkSamples, GST_SECOND, SAMPLE_RATE);
            GST_BUFFER_DURATION(hdtnGstHandoffUtils.buffer) = duration;
            
            gst_buffer_unmap(hdtnGstHandoffUtils.buffer, &hdtnGstHandoffUtils.map);

            /* Push the buffer into the appsrc */
            boost::this_thread::sleep_for(boost::chrono::microseconds(50));
            hdtnGstHandoffUtils.ret = gst_app_src_push_buffer((GstAppSrc *) m_filesinkAppsrc, hdtnGstHandoffUtils.buffer); // takes ownership of buffer we DO NOT deref

            if (hdtnGstHandoffUtils.ret != GST_FLOW_OK) { 
                GST_DEBUG_BIN_TO_DOT_FILE((GstBin *) m_pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "gst_error");
                /* We got some error */
                std::cout << "WARNING: could not push data into app src. Err code: " << hdtnGstHandoffUtils.ret << std::endl;
                continue;
            } 
            m_numFilesinkSamples += 1;
        }

    // if ((m_numFilesinkSamples % 6) == 0) {
    //     static guint bytes_in_appsrc_queue, buffers_in_decodeBuffer, buffersInDisplayQueue, buffersInPostDecodeQueue;
    //     g_object_get(G_OBJECT(m_filesinkAppsrc), "current-level-bytes", &bytes_in_appsrc_queue, NULL);
    //     g_object_get(G_OBJECT(m_filesinkQueue), "current-level-buffers", &buffersInDisplayQueue, NULL);
    //     printf("filesik::bytes_in_appsrc_queue:%u\n", bytes_in_appsrc_queue);
    //     printf("filesik::buffers_in_display_queue:%u\n", buffersInDisplayQueue);
    //     }
    }

    LOG_INFO(subprocess) << "Exiting PushDataToFilesinkThread processing thread";
}

void GStreamerAppSrcOutduct::PushDataToDisplayThread()
{
    static HdtnGstHandoffUtils_t hdtnGstHandoffUtils;
    static padded_vector_uint8_t incomingRtpFrame;
    incomingRtpFrame.reserve(1400);

    while (m_runDisplayThread) {
        
        bool notInWaitForNewPacketsState = m_rtpPacketToDisplayAsyncListenerPtr->TryWaitForIncomingDataAvailable();
        if (notInWaitForNewPacketsState) {
            
            m_rtpPacketToDisplayAsyncListenerPtr->Lock();
                incomingRtpFrame = std::move(m_rtpPacketToDisplayAsyncListenerPtr->m_queue.front());
                m_rtpPacketToDisplayAsyncListenerPtr->PopFront();
            m_rtpPacketToDisplayAsyncListenerPtr->Unlock();
            m_rtpPacketToDisplayAsyncListenerPtr->Notify();

            /* Create a new empty buffer */
            hdtnGstHandoffUtils.buffer = gst_buffer_new_and_alloc(incomingRtpFrame.size());

            /* copy in from our local queue */
            gst_buffer_map(hdtnGstHandoffUtils.buffer, &hdtnGstHandoffUtils.map, GST_MAP_WRITE);
            memcpy(hdtnGstHandoffUtils.map.data, incomingRtpFrame.data(), incomingRtpFrame.size());
         
            /* Set its timestamp and duration */
            GST_BUFFER_PTS(hdtnGstHandoffUtils.buffer) = gst_util_uint64_scale(m_numDisplaySamples, GST_SECOND, SAMPLE_RATE);
            GST_BUFFER_DURATION(hdtnGstHandoffUtils.buffer) = duration;

            // gst_buffer_unmap(hdtnGstHandoffUtils.buffer, &hdtnGstHandoffUtils.map);
            gst_memory_unmap(hdtnGstHandoffUtils.map.memory, &hdtnGstHandoffUtils.map);
            gst_memory_unref(hdtnGstHandoffUtils.map.memory);

            /* Push the buffer into the appsrc */
            // code crashes if we get to push_buffer too quickly (too often?). Need to find a better solution. Crashes reguardless of the method used to push_buffer
            boost::this_thread::sleep_for(boost::chrono::microseconds(50));
            hdtnGstHandoffUtils.ret = gst_app_src_push_buffer((GstAppSrc *) m_displayAppsrc, hdtnGstHandoffUtils.buffer); // takes ownership of buffer we DO NOT deref

            if (hdtnGstHandoffUtils.ret != GST_FLOW_OK) { 
                GST_DEBUG_BIN_TO_DOT_FILE((GstBin *) m_pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "gst_error");
                /* We got some error */
                std::cout << "WARNING: could not push data into app src. Err code: " << hdtnGstHandoffUtils.ret << std::endl;
                continue;
            } 
            m_numDisplaySamples += 1;
        }   

        if ((m_numDisplaySamples % 6) == 0) {
            static guint bytes_in_appsrc_queue, buffers_in_decodeBuffer, buffersInDisplayQueue, buffersInPostDecodeQueue;
            // g_object_get(G_OBJECT(m_displayAppsrc), "current-level-bytes", &bytes_in_appsrc_queue, NULL);
            // g_object_get(G_OBJECT(m_displayQueue), "current-level-buffers", &buffersInDisplayQueue, NULL);
            // g_object_get(G_OBJECT(m_decodeQueue), "current-level-buffers", &buffers_in_decodeBuffer, NULL);
            // g_object_get(G_OBJECT(m_postDecodeQueue), "current-level-buffers", &buffersInPostDecodeQueue, NULL);
            // printf("display::bytes_in_appsrc_queue:%u\n", bytes_in_appsrc_queue);
            // printf("display::buffers_in_display_queue:%u\n", buffersInDisplayQueue);
            // printf("display::buffers_in_decode_queue:%u\n", buffers_in_decodeBuffer);
            // printf("display::buffers_in__post_decode_queue:%u\n", buffersInPostDecodeQueue);
        }
    }

    LOG_INFO(subprocess) << "Exiting PushDataToDisplayThread processing thread";

    return; // get here only when shutting down
}

int GStreamerAppSrcOutduct::PushRtpPacketToGStreamerOutduct(padded_vector_uint8_t &rtpPacketToTake)
{
    m_bundleCallbackAsyncListenerPtr->Lock();
        if (m_bundleCallbackAsyncListenerPtr->m_queue.full())
            m_totalIncomingCbOverruns++;
        m_bundleCallbackAsyncListenerPtr->m_queue.push_back(std::move(rtpPacketToTake));  // copy out bundle to circular buffer for sending
    m_bundleCallbackAsyncListenerPtr->Unlock();
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


