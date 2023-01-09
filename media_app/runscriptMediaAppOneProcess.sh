#!/bin/sh

# path variables
config_files=$HDTN_MEDIA_APP_DIR/config_files
hdtn_config=$config_files/hdtn/hdtn_ingress1tcpclv4_port4556_egress1tcpclv4_port4558flowid2.json
sink_config=$config_files/inducts/bpsink_one_tcpclv4_port4558.json
gen_config=$config_files/outducts/bpgen_one_tcpclv4_port4556.json

cd $HDTN_MEDIA_APP_DIR

# BP Receive File
/home/kyle/nasa/dev/media_app/build/media_app  --save-directory=$HOME/received_media --my-uri-eid=ipn:2.1 --inducts-config-file=$sink_config &
bpreceive_PID=$!
sleep 3

# HDTN one process
./hdtn_build_files/hdtn-one-process --hdtn-config-file=$hdtn_config &
one_process_PID=$!
sleep 6

#Scheduler
./hdtn_build_files/hdtn-scheduler --contact-plan-file=contactPlanCutThroughMode.json --hdtn-config-file=$hdtn_config &
scheduler_PID=$!
sleep 5

# BP Send File 
./hdtn_build_files/bpsendfile  --use-bp-version-7  --max-bundle-size-bytes=4000000 --file-or-folder-path=$HOME/media --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --outducts-config-file=$gen_config &
bpsend_PID=$!
sleep 8

# cleanup
sleep 55
echo "\nkilling bp send file..." && kill -2 $bpsend_PID
sleep 2
echo "\nkilling HDTN one process ..." && kill -2 $one_process_PID
sleep 2
echo "\nkilling HDTN scheduler ..." && kill -2 $scheduler_PID
sleep 2
echo "\nkilling bp receive file..." && kill -2 $bpreceive_PID

