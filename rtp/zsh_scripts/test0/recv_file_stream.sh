#!/bin/sh

# path variables
config_files=$HDTN_RTP_DIR/config_files
# sink_config=$config_files/inducts/ltp_media_sink.json
sink_config=$config_files/inducts/mediasink_stcp.json
# sink_config=$config_files/inducts/mediasink_tcpcl.json

outgoing_rtp_port=60000

cd $HDTN_RTP_DIR 


./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --outgoing-rtp-hostname=127.0.0.1 \
        --outgoing-rtp-port=$outgoing_rtp_port --num-circular-buffer-vectors=500 --max-outgoing-rtp-packet-size-bytes=1472 \
        --ffmpeg-command="\
ffmpeg -y -protocol_whitelist file,udp,rtp,data,sdp \
-strict experimental \
-fflags +genpts \
-seek2any 1 \
-avoid_negative_ts +make_zero \
-reorder_queue_size 0 \
-loglevel verbose \
-fflags nobuffer+fastseek+flush_packets -flags low_delay \
-re -i  \
-vcodec copy -acodec copy \
-f sap sap://224.0.0.255?same_port=1" &

sleep 5

# ffmpeg -y -protocol_whitelist file,udp,rtp \
# -loglevel verbose \
# -strict experimental \
# -fflags +genpts \
# -seek2any 1 \
# -avoid_negative_ts +make_zero \
# # -max_delay 0 \
# # -reorder_queue_size 0 \
# -i HDTN_TO_IN_SDP.sdp -vcodec copy -acodec copy -f mp4 test_output.mp4 & 
       



sleep 5

# cleanup
sleep 360
# echo "\nk

echo "\nkilling HDTN bp receive stream process ..." && kill -2 $stream_recv_id
echo "\nkillingffplay process ..." && kill -2 $ffplay_id





# # sending to vlc
# ffmpeg -y -protocol_whitelist file,udp,rtp,data,sdp \
# -strict experimental \
# -fflags +genpts \
# -seek2any 1 \
# -avoid_negative_ts +make_zero \
# -reorder_queue_size 0 \
# -loglevel verbose \
# -fflags nobuffer+fastseek+flush_packets -flags low_delay \
# -re -i  \
# -vcodec copy -acodec copy \
# -f sap sap://224.0.0.255?same_port=1" &

# # receive stream and save to file
# ffmpeg -y -protocol_whitelist file,udp,rtp \
#         -loglevel verbose \
#         -strict experimental \
#         -fflags +genpts \
#         -seek2any 1 \
#         -avoid_negative_ts +make_zero \
#         -max_delay 0 \
#         -reorder_queue_size 0 \
#         -i HDTN_TO_IN_SDP.sdp -vcodec copy -acodec copy -f mp4 test_output.mp4 & 