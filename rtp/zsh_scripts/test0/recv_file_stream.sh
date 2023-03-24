#!/bin/sh

# path variables
config_files=$HDTN_RTP_DIR/config_files
# sink_config=$config_files/inducts/ltp_media_sink.json
sink_config=$config_files/inducts/mediasink_stcp.json
# sink_config=$config_files/inducts/mediasink_tcpcl.json

outgoing_rtp_port=60000

# # # receive stream and save to file
# ffmpeg -y -protocol_whitelist file,udp,rtp \
#         -strict experimental \
#         -fflags +genpts \
#         -seek2any 1 \
#         -avoid_negative_ts +make_zero \
#         -max_delay 500 \
#         -reorder_queue_size 5 \
#         -loglevel verbose \
#         -i stream_file.sdp -vcodec copy -acodec copy -f mp4 test_output_LTP.MXF & 

# display stream on screen https://stackoverflow.com/questions/47301718/ffmpeg-rtp-streaming-errors-rtp-dropping-old-packet-received-too-late
# ffplay -i stream_file.sdp -protocol_whitelist file,udp,rtp  -reorder_queue_size 0  -fflags nobuffer+fastseek+flush_packets -sync ext -flags low_delay -framedrop &
# ffplay_id=$!
# sleep 1


cd $HDTN_RTP_DIR 

# ./build/bprecv_stream  --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config  --outgoing-rtp-hostname=127.0.0.1 \
#         --outgoing-rtp-port=$outgoing_rtp_port --num-circular-buffer-vectors=500 --max-outgoing-rtp-packet-size-bytes=1472 \
#         --ffmpeg-command="\
#         ffplay -i  -protocol_whitelist data,file,udp,rtp  -reorder_queue_size 0  -fflags nobuffer+fastseek+flush_packets -sync ext -flags low_delay " &
        # ffmpeg -y -protocol_whitelist data,file,udp,rtp \
        # -strict experimental \
        # -fflags +genpts \
        # -seek2any 1 \
        # -avoid_negative_ts +make_zero \
        # -reorder_queue_size 0 \
        # -max_delay 0 \
        # -loglevel verbose \
        # -i  -vcodec copy -acodec copy -f mp4 test_water_stcp.mp4"
# stream_recv_id=$!               

ffmpeg -y -protocol_whitelist data,file,udp,rtp -reorder_queue_size 0 -i HDTN.sdp -f rawvideo -pixel_format yuv422p10le -video_size 3840x2160 output.raw #- | ffplay -protocol_whitelist data,file,udp,rtp,fd -f rawvideo -pixel_format yuv422p10le -video_size 3840x2160 -i -


#  -i water_raw.raw

# ffmpeg -hwaccel cuda -hwaccel_output_format cuda  -protocol_whitelist data,file,udp,rtp -re -listen_timeout -1  -i  HDTN.sdp -c:v copy -reorder_queue_size 0  -fflags nobuffer+fastseek+flush_packets -f matroska - | ffplay -i -   -sync ext -flags low_delay -framedrop

# cleanup
sleep 360
# echo "\nk

echo "\nkilling HDTN bp receive stream process ..." && kill -2 $stream_recv_id
echo "\nkillingffplay process ..." && kill -2 $ffplay_id