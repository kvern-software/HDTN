
# !/bin/sh 

# path variables
config_files=$HDTN_RTP_DIR/config_files
source_config=$config_files/outducts/streamsource_stcp.json

port=50574
udpbuffersize=1400

cd $HDTN_RTP_DIR

# stream send
./build/bpsend_stream  --bundle-size=100000 --bundle-rate=0  \
        --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$source_config \
        --max-incoming-udp-packet-size-bytes=1600 --incoming-rtp-stream-port=$port --num-circular-buffer-vectors=100 & #enlarge buffer a bit +100
media_source_process=$!

sleep 1

# ffmpeg
ffmpeg  -hide_banner -f v4l2 -framerate 30 -video_size 1920x1080 -i /dev/video0 -c:v libx265 -f rtp "rtp://127.0.0.1:$port&buffer_size=$udpbuffersize&localrtpport=51522" -loglevel quiet


sleep 400

echo "\nkilling media source ..." && kill -2 $media_source_process
