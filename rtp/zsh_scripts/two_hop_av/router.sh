
config_files=$HDTN_RTP_DIR/config_files/two_hop_av
hdtn_config=$config_files/central_hdtn_node.json

cd $HDTN_SOURCE_ROOT

# HDTN one process
./build/module/hdtn_one_process/hdtn-one-process  --hdtn-config-file=$hdtn_config &