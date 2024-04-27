#! /bin/bash +x

# Run basic TCP test
clang++ tcp_client_server.cpp -o cs_tcp -std=c++1z

./cs_tcp server & 
./cs_tcp client

# Run iperf single client, bounceback test
# Options bounceback corresponds to equivalent RTT test 
iperf -s &
iperf -c 127.0.0.1 --port 5001 -P 1 -t 90 -l 128 -N -e  --bounceback=1 --bounceback-period=0