#!/bin/sh

# path variables
config_files=$HDTN_MEDIA_APP_DIR/config_files
# hdtn_config=$config_files/hdtn/hdtn_Node1_ltp.json
gen_config=$config_files/outducts/mediasource_udp.json

cd $HDTN_MEDIA_APP_DIR

# HDTN one process
# ./build/module/hdtn_one_process/hdtn-one-process  --hdtn-config-file=$hdtn_config &
# sleep 10

#bpgen
./build/media_source  --frames-per-second=30 --video-device=/dev/video0 --bundle-size=100000 --bundle-rate=0 --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --duration=400 --outducts-config-file=$gen_config &
# sleep 8


