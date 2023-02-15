
# !/bin/sh 

# path variables
config_files=$HDTN_MEDIA_APP_DIR/config_files
hdtn_config=$config_files/hdtn/hdtn_node1.json
source_config=$config_files/outducts/mediasource_udp.json

cd $HDTN_MEDIA_APP_DIR

# ./hdtn_build_files/hdtn-scheduler --contact-plan-file=contactPlanCutThroughMode.json --hdtn-config-file=$hdtn_config &
# scheduler_PID=$!
# sleep 5

# HDTN one process
./hdtn_build_files/hdtn-one-process  --hdtn-config-file=$hdtn_config &
one_process_PID=$!
sleep 6

# media source
./build/media_source  --frames-per-second=30 --video-device=/dev/video0 --bundle-size=200000 --bundle-rate=0 --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --duration=400 --outducts-config-file=$source_config &
media_source_process=$!
# sleep 8

sleep 60

echo "\nkilling HDTN one process ..." && kill -2 $one_process_PID
sleep 2

echo "\nkilling media source ..." && kill -2 $media_source_process


# gdb --args bash media_app_send.sh 