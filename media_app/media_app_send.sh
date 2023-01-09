#!/bin/sh

# path variables
config_files=$HDTN_MEDIA_APP_DIR/config_files
# hdtn_config=$config_files/hdtn/hdtn_Node1_ltp.json
gen_config=$config_files/outducts/filesend_to_port4000_ip_192_168_1_100.json

cd $HDTN_MEDIA_APP_DIR

# HDTN one process
# ./build/module/hdtn_one_process/hdtn-one-process  --hdtn-config-file=$hdtn_config &
# sleep 10

#bpgen
./hdtn_build_files/bpgen-async --bundle-size=100000 --bundle-rate=0 --my-uri-eid=ipn:1.1 --dest-uri-eid=ipn:2.1 --duration=40 --outducts-config-file=$gen_config &
sleep 8


