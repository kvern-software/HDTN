# !/bin/zsh 

config_files=$HDTN_RTP_DIR/config_files/udp/test_2_hdtn_loopback
source_config=$config_files/mediasource_udp.json

test_files=/home/$USER/test_media/official_test_media
filename=water_bubble_cbr21
file=$test_files/$filename.mp4

incoming_rtp_port=29999

cd $HDTN_RTP_DIR


./build/bpsend_stream  --bundle-size=65535 --bundle-rate=0 --use-bp-version-7 \
        --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$source_config \
        --max-incoming-udp-packet-size-bytes=1460 --incoming-rtp-stream-port=$incoming_rtp_port --num-circular-buffer-vectors=2000 \
        --enable-rtp-concatenation=false  --rtp-packets-per-bundle=1 &
media_source_process=$!

sleep 3

####################################################### gst 
gst-launch-1.0 filesrc location=$file ! qtdemux ! h264parse ! rtph264pay config-interval=4 ! udpsink sync=true host=127.0.0.1 port=$incoming_rtp_port 
##################################################### gst


sleep 400

echo "\nkilling video process ..." && kill -2 $media_source_process
kill -2 $ffmpeg_process


