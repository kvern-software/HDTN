#!/bin/sh

# path variables
config_files=$HDTN_MEDIA_APP_DIR/config_files
sink_config=$config_files/inducts/bpsink_one_udp_port4557.json
media_path = $HDTN_MEDIA_APP_DIR/test_media

cd $HDTN_MEDIA_APP_DIR

# Media app start. Media app inherits from BpSinkPattern and functions very similarly to bpreceive file
./build/media_app --save-directory=testdir --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config &
bpreceive_PID=$!
sleep 3

# HDTN one process this is just copy and pasted from the HDTN build dir
# ./build/module/hdtn_one_process/hdtn-one-process --hdtn-config-file=$hdtn_config &
# one_process_PID=$!
# sleep 6

#Scheduler this is just copy and pasted from the HDTN build dir
# ./build/module/scheduler/hdtn-scheduler --contact-plan-file=contactPlanCutThroughMode.json --hdtn-config-file=$hdtn_config &
# scheduler_PID=$!
# sleep 5

# cleanup
# sleep 55
# echo "\nkilling bp send file..." && kill -2 $bpsend_PID
# sleep 2
# echo "\nkilling HDTN one process ..." && kill -2 $one_process_PID
# sleep 2
# echo "\nkilling HDTN scheduler ..." && kill -2 $scheduler_PID
# sleep 2
# echo "\nkilling bp receive file..." && kill -2 $bpreceive_PID


