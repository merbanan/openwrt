#!/bin/sh

IP=192.168.83.115
DST=192.168.83.120
GW=192.168.83.1
DEV=lan1

RATE="100mbit"
BURST="4kb"
LIMIT="4096"

NSTRICT=0
QUANTA="quanta 1514 1514 1514 1514 1514 3528 1514 1514"

PORT0=6001
PRIO0=0

PORT1=6002
PRIO1=5

TIME=30

brctl addbr br0
sleep 1
for i in $(seq 4); do
	ip link set dev lan$i up
	brctl addif br0 lan$i
done
ip a a $IP/24 dev br0
ip link set dev br0 up
ip r a default via $GW
sleep 2
ping -c 10 $DST
sleep 2

#tc filter del dev $DEV egress > /dev/null 2>&1
tc filter del dev $DEV parent 2: > /dev/null 2>&1
tc qdisc replace dev $DEV root handle 1: tbf rate $RATE burst $BURST limit $LIMIT
tc qdisc replace dev $DEV parent 1: handle 2: ets bands 8 strict $NSTRICT $QUANTA
#tc qdisc add dev $DEV clsact
sleep 2

#tc filter add dev $DEV protocol ip egress flower ip_proto tcp dst_port $PORT0 action skbedit priority $PRIO0 flowid 2:$((PRIO0+1))
#tc filter add dev $DEV protocol ip egress flower ip_proto tcp dst_port $PORT1 action skbedit priority $PRIO1 flowid 2:$((PRIO1+1))
tc filter add dev $DEV protocol ip parent 2: flower ip_proto tcp dst_port $PORT0 action skbedit priority $PRIO0 flowid 2:$((PRIO0+1))
tc filter add dev $DEV protocol ip parent 2: flower ip_proto tcp dst_port $PORT1 action skbedit priority $PRIO1 flowid 2:$((PRIO1+1))

echo -e "\n\n********* $DEV *********"
tc qdisc show dev $DEV
tc filter show dev $DEV parent 2:

echo -e "\n\n"
cat /sys/kernel/debug/airoha-eth:1/qos-tx-meters
sleep 10

iperf3 -c $DST -p $PORT0 -t $((TIME*30)) > /dev/null &
for i in $(seq 15); do
	sleep 30
	iperf3 -c $DST -p $PORT1 -t $TIME > /dev/null
done &

while sleep 1; do
	clear
	cat /sys/kernel/debug/airoha-eth:1/qos-tx-counters
	cat /sys/kernel/debug/airoha-eth:1/xmit-rings
done

