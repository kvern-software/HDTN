test_media_folder=/home/kyle/nasa/dev/test_media/official_test_media
filename=lucia_cbr21
file=$test_media_folder/$filename.mp4

output_file=/home/kyle/nasa/dev/test_outputs/test_1
mkdir -p $output_file/$filename

outgoing_rtp_port=5000

#receiver
gst-launch-1.0 -v udpsrc port=$outgoing_rtp_port ! "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" \
! cpureport ! rtph264depay ! h264parse !  qtmux ! filesink location=$output_file/$filename/$filename.mp4 -e &
pid_recv=$!

sleep 1

# sender
gst-launch-1.0 filesrc location=$file ! qtdemux ! h264parse ! rtph264pay config-interval=4 ! queue ! progressreport update-freq=2 ! udpsink host=127.0.0.1 port=$outgoing_rtp_port &
pid_send=$!

sleep 70
kill -2 $pid_recv
kill -2 $pid_send

