#!/bin/sh

# path variables
config_files=$HDTN_RTP_DIR/config_files
sink_config=$config_files/inducts/mediasink_stcp.json
cd $HDTN_RTP_DIR

outgoing_rtp_port=60000
                                                     
ffplay stream_file.sdp -protocol_whitelist file,udp,rtp &
ffplay_id=$!
sleep 1

# Media app start. Media app inherits from BpSinkPattern and functions very similarly to bpreceive file
./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --remote-hostname=192.168.1.132 \
        --outgoing-rtp-port=$outgoing_rtp_port --num-circular-buffer-vectors=50 --max-outgoing-rtp-packet-size-bytes=1472
stream_recv_id=$!               



# cleanup
sleep 360
# echo "\nk

echo "\nkilling HDTN bp receive stream process ..." && kill -2 $stream_recv_id
echo "\nkillingffplay process ..." && kill -2 $ffplay_id