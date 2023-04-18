test_media_folder=/home/kyle/nasa/dev/test_media/official_test_media
filename=water_bubble_h264_crf_21
file=$test_media_folder/$filename.mp4

# filename=ammonia_trimmed
# file=$test_media_folder/$filename.wav

output_file=/home/kyle/nasa/dev/test_outputs/test_1
mkdir $output_file/$filename

# change this if sending video or audio
audio_only="-c:a aac -b:a 96k -vn -f flv"
video_only="-c copy -an"
ffmpeg_command_slice=$video_only

# sender video
# ffmpeg -y -sdp_file HDTN.sdp -re -i $file -c copy -an -f rtp "rtp://127.0.0.1:50000" &


# sender audio
ffmpeg -y -sdp_file HDTN.sdp -re -i $file $ffmpeg_command_slice -f rtp "rtp://127.0.0.1:50000" &
ffmpeg_process=$!

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
        -f mp4 $output_file/$filename/$filename.mp4 &
        # -f flv $output_file/$filename/$filename.flv &


sleep 500
