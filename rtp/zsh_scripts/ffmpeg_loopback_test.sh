test_media_folder=/home/kyle/nasa/dev/test_media/official_test_media
filename=water_bubble_h264_crf_18
file=$test_media_folder/$filename.mp4

mkdir /home/kyle/nasa/dev/test_outputs/$filename

# sender
ffmpeg -y -sdp_file HDTN.sdp -re -i $file -c copy -an -f rtp "rtp://127.0.0.1:50000" &

# receiver
ffmpeg  -y -protocol_whitelist file,udp,rtp \
        -strict experimental \
        -fflags +genpts \
        -seek2any 1 \
        -avoid_negative_ts +make_zero \
        -reorder_queue_size 0 \
        -loglevel verbose \
        -fflags nobuffer+fastseek+flush_packets -flags low_delay \
        -i HDTN.sdp \
        -c:v copy -c:a copy \
        -f mp4 /home/kyle/nasa/dev/test_outputs/$filename/$filename.mp4 &


sleep 500
