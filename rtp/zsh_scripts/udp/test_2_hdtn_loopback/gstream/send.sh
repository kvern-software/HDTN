# !/bin/zsh 

# config_files=$HDTN_RTP_DIR/config_files/udp/test_2_hdtn_loopback
config_files=$HDTN_RTP_DIR/config_files/ltp/test_2_hdtn_loopback
source_config=$config_files/mediasource_ltp.json
# source_config=$config_files/mediasource_udp.json
# source_config=$config_files/mediasource_stcp.json

test_files=/home/$USER/test_media/official_test_media
# filename=water_bubble_cbr21
filename=new_media/water_bubble_crf18
# filename=lucia_cbr21
# filename=lucia_crf18
file=$test_files/$filename.mp4

incoming_rtp_port=30000

shm_socket_path=/tmp/hdtn_gst_shm_induct_socket

cd $HDTN_RTP_DIR

rm $shm_socket_path*
pkill -9 BpSendStream

sleep 0.5

./build/bpsend_stream  --bundle-size=65535 --bundle-rate=0 --use-bp-version-7 \
        --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$source_config \
        --num-circular-buffer-vectors=10000 --rtp-packets-per-bundle=5 --max-incoming-udp-packet-size-bytes=1460\
        --induct-type="appsink" --file-to-stream=$file  &
        # --induct-type="udp" --incoming-rtp-stream-port=$incoming_rtp_port &  
bpsend_stream_process=$!


sleep 65

echo "\nkilling video process ..." && pkill -15 BpSendStream


# gst-launch-1.0 rtspsrc location="rtsp://10.54.31.253:554/cam/realmonitor?channel=1&subtype=0&unicast=false" user-id=admin user-pw=nasa1234 ! queue ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! video/x-raw,width=3840,height=2160 ! autovideosink
#gst-launch-1.0 rtspsrc location="rtsp://10.54.31.253:554/cam/realmonitor?channel=1&subtype=0&unicast=false" user-id=admin user-pw=nasa1234 ! queue ! udpsink host=127.0.0.1 port=30000 


