#!/bin/bash

NIC=$1
if [ ! $1 ]; then
    NIC='eth0'
fi

COMPARE=compare
if [ "$2" == "nocompare" ]; then
    COMPARE=nocompare
fi

rm -rf gredump.pcap
rm -rf dump

tcpreplay --pps=50 -i $NIC xml.pcap &

../bin/gredump -i $NIC -o gredump.pcap &
../bin/pktminerg "-i $NIC -r 172.16.14.249 -c 1000 --dump ./dump/ --interval 0" &

sleep 30

killall tcpreplay
killall pktminerg
killall gredump

if [ "$COMPARE" == "compare" ]; then
    ../bin/pcapcompare ./dump/$NIC/pktminerg_dump.pcap gredump.pcap
fi


