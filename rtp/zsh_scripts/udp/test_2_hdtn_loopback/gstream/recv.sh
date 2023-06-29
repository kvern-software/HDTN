config_files=$HDTN_RTP_DIR/config_files/stcp/test_2_hdtn_loopback
# sink_config=$config_files/mediasink_udp.json
sink_config=$config_files/mediasink_stcp.json

outgoing_rtp_port=40004 

output_file_path="/home/$USER/test_outputs/test_2"
# filename=lucia_cbr21                 # change this for whatever file you want to name
filename=lucia_crf18
file=$output_file_path/$filename


shm_socket_path=/tmp/hdtn_gst_shm_outduct

mkdir -p  $output_file_path/$filename

cd $HDTN_RTP_DIR 

pkill -9 BpRecvStream
pkill -9 gst-launch-1.0

# rm /tmp/gst_log.log 
export GST_DEBUG_FILE=/tmp/gst_log.log
#################################################################################
echo "Deleting old socket file: "
echo $shm_socket_path
rm $shm_socket_path
#################################################################################
echo "Deleting old output file: "
echo $file/$filename.mp4
rm $file/$filename.mp4
sleep .5
#################################################################################
export GST_DEBUG=3,queue:3,appsrc:3,shmsink:3,decodebin:5
# gdb -ex run --args ./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --num-circular-buffer-vectors=10000 \
#         --max-outgoing-rtp-packet-size-bytes=1460 --outduct-type="appsrc" --shm-socket-path=$shm_socket_path 
./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --num-circular-buffer-vectors=10000 \
        --max-outgoing-rtp-packet-size-bytes=1460 --outduct-type="appsrc" --shm-socket-path=$shm_socket_path &
bprecv_stream_pid=$!
#################################################################################
sleep 3
#################################################################################
# if we are using appsrc, launch a separate gstreamer instance to save the video stream to file 
# export GST_DEBUG=3,shmsrc:6
gst-launch-1.0 shmsrc socket-path=$shm_socket_path  is-live=true do-timestamp=true \
        ! "video/x-raw, format=(string)I420, width=(int)3840, height=(int)2160, framerate=(fraction)60000/1001" \
        ! queue ! glupload ! glimagesink  &
# ! queue ! fakesink dump=true  & #! "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" \
#   ! queue ! rtph264depay ! h264parse ! mp4mux ! filesink location=$file/$filename.mp4 -e  &
# gst-launch-1.0 shmsrc socket-path=$shm_socket_path  is-live=true do-timestamp=true ! queue ! fakesink  &
sleep 70 


#################################################################################
# pkill -9 gst-launch-1.0
pkill -15 BpRecvStream
