config_files=$HDTN_RTP_DIR/config_files/udp/test_3_point_to_point
sink_config=$config_files/mediasink_udp.json

outgoing_rtp_port=40004 

output_file_path="/home/$USER/test_outputs/test_3_jetson_to_pc"
filename=lucia_crf18_g_15_newQueue           # change this for whatever file you want to name
file=$output_file_path/$filename

mkdir -p  $output_file_path/$filename/$filename

cd $HDTN_RTP_DIR 

./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --outgoing-rtp-hostname=127.0.0.1 \
        --outgoing-rtp-port=$outgoing_rtp_port --num-circular-buffer-vectors=2000 --max-outgoing-rtp-packet-size-bytes=1460 & 
recv_pid=$!

gst-launch-1.0 -v udpsrc port=$outgoing_rtp_port buffer-size=1800 ! "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" \
! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! rtph264depay ! h264parse !  mp4mux ! filesink location=$file/$filename/$filename.mp4  -e  &
# ! queue max-size-buffers=500 ! rtpjitterbuffer max-misorder-time=5000 latency=1000 max-dropout-time=10000 \
gst_pid=$!


sleep 80

kill -2 $recv_pid
kill -2 $gst_pid
