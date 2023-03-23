# !/bin/zsh 

config_files=$HDTN_RTP_DIR/config_files/point_to_point
# source_config=$config_files/ltp_media_source.json
source_config=$config_files/mediasource_stcp.json
# source_config=$config_files/mediasource_tcpcl.json

incoming_rtp_port=40002

cd $HDTN_RTP_DIR

./build/bpsend_stream  --bundle-size=2000 --bundle-rate=0 --use-bp-version-7 \
        --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$source_config \
        --max-incoming-udp-packet-size-bytes=1600 --incoming-rtp-stream-port=$incoming_rtp_port --num-circular-buffer-vectors=500 \
        --enable-rtp-concatenation=false --sdp-filepath="HDTN.sdp" &
media_source_process=$!