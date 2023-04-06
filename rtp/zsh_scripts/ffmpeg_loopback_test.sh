test_media_folder=/home/kyle/nasa/dev/test_media/official_test_media
file=$test_media_folder/water_bubble.mp4

# sender
ffmpeg -y -sdp_file HDTN.sdp -re -i $file -c copy -an -f rtp "rtp://127.0.0.1:50000" &

# receiver
ffmpeg  -hwaccel cuda -hwaccel_output_format cuda \
        -protocol_whitelist file,udp,rtp \
        -strict experimental \
        -fflags +genpts \
        -seek2any 1 \
        -avoid_negative_ts +make_zero \
        -reorder_queue_size 0 \
        -loglevel verbose \
        -fflags nobuffer+fastseek+flush_packets -flags low_delay \
        -i HDTN.sdp \
        -f matroska - | ffplay -i -


sleep 500
