config_files=$HDTN_RTP_DIR/config_files/udp/test_2_hdtn_loopback
sink_config=$config_files/mediasink_udp.json

outgoing_rtp_port=40004 

output_file_path="/home/$USER/gstream/test_2/udp_rate_limiting"
filename=water_bubble_h264_vbr_g_15                 # change this for whatever file you want to name
file=$output_file_path/$filename

mkdir -p  $output_file_path/$filename

cd $HDTN_RTP_DIR 

./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --outgoing-rtp-hostname=127.0.0.1 \
        --outgoing-rtp-port=$outgoing_rtp_port --num-circular-buffer-vectors=10000 --max-outgoing-rtp-packet-size-bytes=1800 & 

gst-launch-1.0 -v udpsrc port=$outgoing_rtp_port ! "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" \
! rtph264depay ! h264parse !  mp4mux ! filesink location=$file/$filename.mp4  -e 
# queue max-size-buffers=500 ! rtpjitterbuffer max-misorder-time=5000 latency=1000 max-dropout-time=10000 ! 

recv_pid=$!


sleep 400

kill -2 $recv_pid
