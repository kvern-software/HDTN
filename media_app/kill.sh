#!/bin/bash
for j in hdtn-one-process bpsendfile bpreceivefile hdtn-scheduler media_source media_sink
do
for i in `pidof $j`
do
 kill -9 $i
done
done

sleep 1

for j in hdtn-one-process bpsendfile bpreceivefile hdtn-scheduler media_source media_sink
do
for i in `pidof $j`
do
 kill -9 $i
done
done

