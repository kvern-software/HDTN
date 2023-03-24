# !/bin/zsh 
test_media_folder=/home/kyle/nasa/dev/test_media/official_test_media
# file=$test_media_folder/water_bubble.mp4
file=$test_media_folder/water_bubble_264.MKV

rtp_port=40002
raspberrypi_ip=192.168.1.172

# must copy over HDTN.sdp manually
ffmpeg -y -sdp_file HDTN.sdp -re -i $file -c copy -an -f rtp "rtp://$raspberrypi_ip:$rtp_port" 

