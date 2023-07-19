#!/bin/sh
#run this on gateway 
# path variables
config_files=$HDTN_RTP_DIR/config_files/ltp/test_6_luna_net
hdtn_config=$config_files/hdtn_one_process_node_2.json
contact_plan=$HDTN_RTP_DIR/config_files/contact_plans/LunaNetContactPlanNodeIDs.json

cd $HDTN_SOURCE_ROOT

# HDTN one process
./build/module/hdtn_one_process/hdtn-one-process  --contact-plan-file=$contact_plan --hdtn-config-file=$hdtn_config &
one_process_pid=$!

sleep 360

echo "\nkilling hdtn one process" && kill -2 $one_process_pid