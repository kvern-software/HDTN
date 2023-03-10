# !/bin/sh 

# path variables
config_files=$HDTN_RTP_DIR/config_files
source_config=$config_files/outducts/streamsource_stcp.json
file=test_media/ISS_View_of_Planet_Earth_2160p.mp4

port=50574

cd $HDTN_RTP_DIR

# stream send
./build/bpsend_stream  --bundle-size=100000 --bundle-rate=0  \
        --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$source_config \
        --max-incoming-udp-packet-size-bytes=1600 --incoming-rtp-stream-port=$port --num-circular-buffer-vectors=100 \
        --enable-rtp-concatenation=false & #enlarge buffer a bit +100
media_source_process=$!

sleep 1

ffmpeg -hwaccel cuda -hwaccel_output_format cuda  -re  -i $file \
                -c:a copy -c:v hevc_nvenc -f rtp "rtp://127.0.0.1:$port"


sleep 400

echo "\nkilling media source ..." && kill -2 $media_source_process
