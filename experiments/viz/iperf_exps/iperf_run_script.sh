#!/bin/bash -x

### Run Parameters
###      iperf -c 127.0.0.1 --port 5001 -P 1 -t 30 -l 128   -N -e --bounceback=1 --bounceback-period=0
host=127.0.0.1
port=8888
payloadBufSize=4
numClients=25
runtime=15
message_size=128
num_burst_messages=1
bounceback_period=0


### make data dir
mkdir data

### Run Server
taskset -c 0-7 iperf --port $port -s -e  &
 # chrt -d 0 iperf --port $port -s -e  &
# iperf --port $port -s -e  &


### Run Client

for num in $(seq 2 $numClients)
do
    filename="iperf_${num}.txt"
    taskset -c 8-15 iperf -c $host --port $port -P $num -t $runtime -l $message_size  -N -e --bounceback=$num_burst_messages --bounceback-period=$bounceback_period  > "data/"$filename
    # iperf -c $host --port $port -P $num -t $runtime -l $message_size  -N -e --bounceback=$num_burst_messages --bounceback-period=$bounceback_period  > "data/"$filename
done
