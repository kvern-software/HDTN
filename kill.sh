#!/bin/bash

for j in hdtn-one-process hdtn-ingress hdtn-egress-async hdtn-storage hdtn-scheduler bpsink-async bp  bpgen-async hdtn-router 
do
for i in `pidof $j`
do
 kill -SIGINT $i
done
done

sleep 6

for j in hdtn-one-process hdtn-ingress hdtn-egress-async hdtn-storage hdtn-scheduler bpsink-async bpgen-async hdtn-router

do
for i in `pidof $j`
do
 kill -9 $i
done
done
