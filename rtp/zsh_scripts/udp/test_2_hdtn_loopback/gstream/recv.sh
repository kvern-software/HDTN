config_files=$HDTN_RTP_DIR/config_files/udp/test_2_hdtn_loopback
sink_config=$config_files/mediasink_udp.json

outgoing_rtp_port=40004 

output_file_path="/home/$USER/test_outputs/test_2"
filename=lucia_cbr21                 # change this for whatever file you want to name
file=$output_file_path/$filename


shm_socket_path=/tmp/hdtn_gst_shm_outduct

mkdir -p  $output_file_path/$filename

cd $HDTN_RTP_DIR 

pkill -9 gst-launch-1.0
rm $shm_socket_path

./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --num-circular-buffer-vectors=10000 \
        --max-outgoing-rtp-packet-size-bytes=1460 --outduct-type="appsrc" --shm-socket-path=$shm_socket_path & 
bprecv_stream_pid=$!

sleep 2

# if we are using appsrc, launch a separate gstreamer instance to save the video stream to file 
# export GST_DEBUG=5
gst-launch-1.0 shmsrc socket-path=$shm_socket_path  is-live=true do-timestamp=true  ! "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" \
  fakesink dump=true & # rtph264depay ! h264parse ! decodebin ! videoconvert ! glimagesink &
# gst-launch-1.0 shmsrc socket-path=$shm_socket_path is-live=true do-timestamp=true !  "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" ! \
        # queue ! rtph264depay ! h264parse ! decodebin ! glimagesink & #filesink location=$file/$filename.mp4 -e &
# decodebin ! videoconvert ! glupload ! glimagesink async=true async-handling=true max-lateness=9223372036854775807

# gst-launch-1.0 -v udpsrc port=$outgoing_rtp_port ! "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" \
# ! rtph264depay ! h264parse !  mp4mux ! filesink location=$file/$filename.mp4  -e  &



sleep 80

kill -2 $bprecv_stream_pid
