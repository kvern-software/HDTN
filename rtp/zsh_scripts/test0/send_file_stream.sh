# !/bin/sh 

# path variables
config_files=$HDTN_RTP_DIR/config_files
# source_config=$config_files/outducts/ltp_media_source.json
source_config=$config_files/outducts/mediasource_stcp.json
# source_config=$config_files/outducts/mediasource_tcpcl.json

test_media_folder=/home/kyle/nasa/dev/test_media/official_test_media
# file=$test_media_folder/../GMT326_17_58_Josh_Cassada_1458_HAM_with_St_Lucia.mp4
# file=$test_media_folder/../ISS_Views_With_Music.mp4
# file=$test_media_folder/Surface_Tension.MKV
# file=$test_media_folder/A012C002H2201038S_CANON_01-Surface_Tension.MXF
# file=$test_media_folder/ammonia_trimmed.flac
# file=$test_media_folder/water_bubble.mp4
file=$test_media_folder/water_raw.raw


rtp_port=40002

cd $HDTN_RTP_DIR

# rm HDTN.sdp

# ffmpeg_sdp_process=$!
# sleep 0.5
# kill -2 $ffmpeg_sdp_process



# stream send
# ./build/bpsend_stream  --bundle-size=2000 --bundle-rate=0 --use-bp-version-7 \
#         --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$source_config \
#         --max-incoming-udp-packet-size-bytes=1600 --incoming-rtp-stream-port=$rtp_port --num-circular-buffer-vectors=5000 \
#         --enable-rtp-concatenation=false --sdp-filepath="HDTN.sdp" &
# media_source_process=$!

# sleep 3
# ffmpeg -y -sdp_file HDTN.sdp -re -i $file -c copy -an -f rtp "rtp://127.0.0.1:$rtp_port" &
ffmpeg -y -sdp_file HDTN.sdp -re -f rawvideo -pixel_format yuv422p10le -video_size 3840x2160 -i $file -c:v copy -an -f rtp "rtp://127.0.0.1:$rtp_port" &

# ffmpeg -y -re -i $file -c copy  -an -f rtp "rtp://127.0.0.1:$rtp_port"  &


sleep 400

echo "\nkilling media source ..." && kill -2 $media_source_process
