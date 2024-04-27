#!/bin/bash -x

### Run Parameters
###      iperf -c 127.0.0.1 --port 5001 -P 1 -t 30 -l 128   -N -e --bounceback=1 --bounceback-period=0
host=127.0.0.1
port=5001
payloadBufSize=4
numClients=25
runtime=90
message_size=128
num_burst_messages=1
bounceback_period=0


### Run Server
iperf -s  &

### Run Client

for num in $(seq 1 $numClients)
do
    filename="iperf_${num}.csv"
    iperf -c $host --port $port -P $num -t $runtime -l $message_size  -N -e --bounceback=$num_burst_messages --bounceback-period=$bounceback_period > $filename
done
