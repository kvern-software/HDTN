# send a file using nvidia hardware  h265 encoding to an rtp stream. -re to enable real time mode
ffmpeg -hwaccel cuda -hwaccel_output_format cuda  -re  -i test_media/ISS_View_of_Planet_Earth_2160p.mp4 \
                -c:a copy -c:v hevc_nvenc -f rtp "rtp://127.0.0.1:50574"
#receive that file using ffplay -> copy the SDP text from the output of the above command into a file called file_sdp.sdp then run
ffplay file_sdp.sdp  -protocol_whitelist file,udp,rtp

# send webcam using nvidia hardware h265 encoding to an rtp stream
 ffmpeg -hwaccel cuda -hwaccel_output_format cuda  \
    -f v4l2 -framerate 30 -video_size 1280x720 -i /dev/video0 \
    -c:v hevc_nvenc -preset p1 -tune ull  -muxpreload 0 -muxdelay 0 -rc cbr -cbr true \
    -f rtp "rtp://127.0.0.1:60000"


##eceive that stream using ffplay -> copy the SDP text from the output of the above command into a file called webcame_sdp.sdp then run
ffplay webcam_sdp.sdp -protocol_whitelist file,udp,rtp



#  get all supported formats
v4l2-ctl --list-formats-ext 

#nvidia codec options
https://gist.github.com/nico-lab/c2d192cbb793dfd241c1eafeb52a21c3

# for psnr and ssim ensure that the videos are same framerate and scaled correctly

# psnr
ffmpeg \
    -i transmitted.mp4 \
    -i original.mp4 \
    psnr="psnr.log" \ 
    -f null -

# ssim
ffmpeg \
    -i transmitted.mp4 \
    -i original.mp4 \
    ssim="ssim.log" \ 
    -f null -

# get last 60 seconds of  file into new file
ffmpeg -sseof -60 -i A012C002H2201038S_CANON_01-Surface_Tension.MXF -c copy tesntion_out.MKV            
ffmpeg -ss 00:04:18 -i A012C002H2201038S_CANON_01-Surface_Tension.MXF -t 60 -c copy trimmed.MKV
