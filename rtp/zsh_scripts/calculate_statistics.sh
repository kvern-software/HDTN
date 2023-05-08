# step one: make sure all file names and paths are correct
# step two: compute time difference between the files using ffmpeg
# step three: use ffmpeg to compute PSNR and SSIM 
# step four: use easy vmaf to compute vmaf. be sure to set sw to the time difference to reduce computation time
# step five: plot the data using python scripts

# step one
test_number=test_4
test_name=water_bubble_crf18

test_media_folder=/home/$USER/nasa/dev/test_media/official_test_media 
reference_file=$test_media_folder/$test_name.mp4

distorted_media_folder=/home/$USER/nasa/dev/test_outputs/$test_number/$test_name
distorted_filename=$test_name
distorted_file=$distorted_media_folder/$distorted_filename.mp4
distorted_file_prepared=$distorted_media_folder/$distorted_filename"_prepared".mp4


 # step two
reference_length=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 $reference_file)
transmitted_length=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 $distorted_file)
delta_time=$(echo "$reference_length - $transmitted_length"|bc)
margin=0.01
trim_time=$(echo "$delta_time - $margin"|bc)
window_time=0.05

echo "reference length: " $reference_length
echo "transmitted length: "$transmitted_length
echo "delta length: "$delta_time
echo "trim length: "$trim_time
echo "psnr window length: "$window_time
 
# step three
cd $distorted_media_folder
ffmpeg -y -i $distorted_file -ss 0$delta_time -i $reference_file -lavfi "[0:v]settb=AVTB,setpts=PTS-STARTPTS[main];[1:v]settb=AVTB,setpts=PTS-STARTPTS[ref];[main][ref]psnr=stats_file=psnr_logfile.txt" -f null - 
ffmpeg -y -i $distorted_file -ss 0$delta_time -i $reference_file -lavfi "[0:v]settb=AVTB,setpts=PTS-STARTPTS[main];[1:v]settb=AVTB,setpts=PTS-STARTPTS[ref];[main][ref]ssim=stats_file=ssim_logfile.txt" -f null - 

# step four
cd /home/$USER/third_party/easyVmaf
python3 easyVmaf.py -d  $distorted_file -r $reference_file \
-subsample 10 \
-sw $window_time \
-model 4K \
-threads 15 \
-progress  \
-endsync  \
-fps 59.94 \
-output_fmt json 

#   step five
cd $HDTN_RTP_DIR/python
python3 plot_ssim.py $test_number/$test_name
python3 plot_psnr.py $test_number/$test_name
python3 plot_vmaf.py $test_number/$test_name $test_name
echo "Finished computing $test_name statistics."