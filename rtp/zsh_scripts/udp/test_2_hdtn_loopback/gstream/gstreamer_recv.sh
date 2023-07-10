config_files=$HDTN_RTP_DIR/config_files/stcp/test_2_hdtn_loopback
# sink_config=$config_files/mediasink_udp.json
sink_config=$config_files/mediasink_stcp.json


output_file_path="/home/$USER/test_outputs/test_2"
# filename=lucia_cbr21                 # change this for whatever file you want to name
filename=lucia_crf18
file=$output_file_path/$filename

shm_socket_path=/tmp/hdtn_gst_shm_outduct

mkdir -p  $output_file_path/$filename

cd $HDTN_RTP_DIR 

export GST_DEBUG_FILE=/tmp/gst_log.log

# if we are using appsrc, launch a separate gstreamer instance to save the video stream to file 
export GST_DEBUG=3,shmsrc:3
gdb -ex run --args gst-launch-1.0 shmsrc socket-path=$shm_socket_path  is-live=true do-timestamp=true ! queue ! fakesink  #! "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" \
        # ! queue ! rtph264depay ! h264parse ! decodebin ! videoconvert ! glupload ! glimagesink  &
#   ! queue ! rtph264depay ! h264parse ! mp4mux ! filesink location=$file/$filename.mp4 -e  &
# gst-launch-1.0 shmsrc socket-path=$shm_socket_path  is-live=true do-timestamp=true ! queue ! fakesink  &
sleep 70 

