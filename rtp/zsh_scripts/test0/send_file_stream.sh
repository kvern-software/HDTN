# !/bin/sh 

# path variables
config_files=$HDTN_RTP_DIR/config_files
# source_config=$config_files/outducts/ltp_media_source.json
source_config=$config_files/outducts/mediasource_stcp.json
# source_config=$config_files/outducts/mediasource_tcpcl.json

test_media_folder=/home/kyle/nasa/dev/test_media/official_test_media
file=$test_media_folder/lucia.mp4



rtp_port=40002

cd $HDTN_RTP_DIR

ffmpeg -y -sdp_file HDTN.sdp -re -i $file -c copy -an -f rtp "rtp://127.0.0.1:$rtp_port" &
ffmpeg_process=$!
# pause ffmpeg to let HDTN start
kill -s STOP $ffmpeg_process

# stream send
./build/bpsend_stream  --bundle-size=2000 --bundle-rate=0 --use-bp-version-7 \
        --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$source_config \
        --max-incoming-udp-packet-size-bytes=1600 --incoming-rtp-stream-port=$rtp_port --num-circular-buffer-vectors=3000 \
        --enable-rtp-concatenation=false --sdp-filepath="HDTN.sdp" &
media_source_process=$!

sleep 5

# resume ffmpeg
kill -s CONT $ffmpeg_process

# ffmpeg -y -re -i $file -c copy  -an -f rtp "rtp://127.0.0.1:$rtp_port"  &


# ffmpeg -y -sdp_file HDTN.sdp -re -f rawvideo -pixel_format yuv422p10le -video_size 3840x2160 -i $file -c:v copy -an -f rtp "rtp://127.0.0.1:$rtp_port" &



sleep 400

echo "\nkilling media source ..." && kill -2 $media_source_process
