cd ~/

# run scripts
sshpass -p "temppwd"  scp -r HDTN/rtp/zsh_scripts/ltp/lunanet_test rover@rover.local:/home/rover/HDTN/rtp/zsh_scripts/ltp/
sshpass -p "temppwd"  scp -r HDTN/rtp/zsh_scripts/ltp/lunanet_test gateway@gateway.local:/home/gateway/HDTN/rtp/zsh_scripts/ltp/
sshpass -p "temppwd"  scp -r HDTN/rtp/zsh_scripts/ltp/lunanet_test madrid@madrid.local:/home/madrid/HDTN/rtp/zsh_scripts/ltp/
sshpass -p "temppwd"  scp -r HDTN/rtp/zsh_scripts/ltp/lunanet_test relay@relay.local:/home/relay/HDTN/rtp/zsh_scripts/ltp/
sshpass -p "temppwd"  scp -r HDTN/rtp/zsh_scripts/ltp/lunanet_test nsn@nsn.local:/home/nsn/HDTN/rtp/zsh_scripts/ltp/
sshpass -p "jetson"  scp -r HDTN/rtp/zsh_scripts/ltp/lunanet_test jetson@10.1.1.70:/home/jetson/HDTN/rtp/zsh_scripts/ltp/

echo "Updated all .sh runscripts for LunaNet test"

# config files
sshpass -p "temppwd" scp -r HDTN/rtp/config_files/ltp/test_6_luna_net rover@rover.local:/home/rover/HDTN/rtp/config_files/ltp/
sshpass -p "temppwd" scp -r HDTN/rtp/config_files/ltp/test_6_luna_net gateway@gateway.local:/home/gateway/HDTN/rtp/config_files/ltp/
sshpass -p "temppwd" scp -r HDTN/rtp/config_files/ltp/test_6_luna_net madrid@madrid.local:/home/madrid/HDTN/rtp/config_files/ltp/
sshpass -p "temppwd" scp -r HDTN/rtp/config_files/ltp/test_6_luna_net relay@relay.local:/home/relay/HDTN/rtp/config_files/ltp/
sshpass -p "temppwd" scp -r HDTN/rtp/config_files/ltp/test_6_luna_net nsn@nsn.local:/home/nsn/HDTN/rtp/config_files/ltp/
sshpass -p "jetson" scp -r HDTN/rtp/config_files/ltp/test_6_luna_net jetson@10.1.1.70:/home/jetson/HDTN/rtp/config_files/ltp/

echo "Updated all .json config files for LunaNet test"


# contact plan
sshpass -p "temppwd" scp HDTN/rtp/config_files/contact_plans/LunaNetContactPlanNodeIDs.json rover@rover.local:/home/rover/HDTN/rtp/config_files/contact_plans
sshpass -p "temppwd" scp HDTN/rtp/config_files/contact_plans/LunaNetContactPlanNodeIDs.json gateway@gateway.local:/home/gateway/HDTN/rtp/config_files/contact_plans
sshpass -p "temppwd" scp HDTN/rtp/config_files/contact_plans/LunaNetContactPlanNodeIDs.json madrid@madrid.local:/home/madrid/HDTN/rtp/config_files/contact_plans
sshpass -p "temppwd" scp HDTN/rtp/config_files/contact_plans/LunaNetContactPlanNodeIDs.json relay@relay.local:/home/relay/HDTN/rtp/config_files/contact_plans
sshpass -p "temppwd" scp HDTN/rtp/config_files/contact_plans/LunaNetContactPlanNodeIDs.json nsn@nsn.local:/home/nsn/HDTN/rtp/config_files/contact_plans
sshpass -p "jetson" scp HDTN/rtp/config_files/contact_plans/LunaNetContactPlanNodeIDs.json jetson@10.1.1.70:/home/jetson/HDTN/rtp/config_files/contact_plans
echo "Updated contact plan for LunaNet test"




# source code 
# sshpass -p "temppwd" scp -r HDTN/rtp/BpInduct rover@rover.local:/home/rover/HDTN/rtp