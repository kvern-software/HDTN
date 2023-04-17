# step one
    # make sure all file names and paths are correct
# step two
    # compute time difference between the files using ffmpeg
# step three
    # use easy vmaf to compute vmaf. be sure to set sw to the time difference to reduce computation time
# step four
    # use ffmpeg to compute PSNR and SSIM 
# step five
    # plot the data using python scripts

# step one
test_name=water_bubble_h264_crf_18

test_media_folder=/home/kyle/nasa/dev/test_media/official_test_media 
reference_file=$test_media_folder/$test_name.mp4

distorted_media_folder=/home/kyle/nasa/dev/test_outputs/$test_name
distorted_filename=$test_name
distorted_file=$distorted_media_folder/$distorted_filename.mp4

# step two
reference_length=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 $reference_file)
transmitted_length=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 $distorted_file)
delta_time=$(echo "$reference_length - $transmitted_length"|bc)
echo $reference_length
echo $transmitted_length
echo $delta_time
margin=0.2
window_time=$(echo "$delta_time + $margin"|bc)
echo $window_time

# step three
cd /home/kyle/third_party/easyVmaf
python3 easyVmaf.py -d  $distorted_file -r $reference_file \
-subsample 10 \
-sw $window_time \
-model 4K \
-threads 15 \
-progress  \
-endsync  \
-output_fmt json 

cd $distorted_media_folder

# step four
# ffmpeg -i $distorted_file -ss 0.950955 -i $reference_file -lavfi psnr=stats_file=psnr_logfile.txt -f null - 
# ffmpeg -i $distorted_file -ss 0.950955 -i $reference_file -lavfi ssim=stats_file=ssim_logfile.txt -f null - 

# step five
cd $HDTN_RTP_DIR/python
python3 plot_ssim.py $test_name
python3 plot_psnr.py $test_name
python3 plot_vmaf.py $test_name


 


# ffmpeg \
# -r 60000/1001 -i $reference_file \
# -r 60000/1001 -i $distorted_file \
# -lavfi "\
#     --pixel_format 420 \
#     -w 3840 -h 2160 \
#     --csv \
#     -o $distorted_media_folder/vmaf_outputs/$distorted_filename \
#     --bitdepth 8 \
#     --subsample 2 \
#     --model version="vmaf_4k_v0.6.1" \
#     --feature psnr \
#     --feature float_ssim"


# vmaf -r $reference_file -d $reference_file \
#     --pixel_format 420 \
#     -w 3840 -h 2160 \
#     --bitdepth 8 \
#     --csv \
#     -o $distorted_media_folder/vmaf_outputs/$distorted_filename \
#     --subsample 2 \
#     --model version="vmaf_4k_v0.6.1" \
#     --feature psnr \
#     --feature float_ssim \
