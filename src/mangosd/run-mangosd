#!/bin/bash
# Massive Network Game Object Server
# autorestart Script

while :
do
	echo "MaNGOS daemon restarted"
	echo `date` >> crash.log &
	./mangosd | tail -n 20 >> crash.log
	echo " " >> crash.log &
	pid=`ps ax | awk '($5 ~ /mangosd/) { print $1 }'`
	wait $pid
        echo `date` ", MaNGOS daemon crashed and restarted." >> serverlog
done
