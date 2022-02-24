#!/bin/bash

# pressure test using multi-thread
# usage: loop, may from 0-3, 4-6, 7-10

# ./pressuretest.sh 0 3
for (( i=$1;i<$2;i++ ))
do
   ./test.sh "noproxy_$i" "proxy_$i" &
   ./testPost.sh &
done

wait

# if do not want results, uncomment those two lines
#rm -rf proxydir  noproxy	# test dir
#rm -rf noproxy_* proxy_*	# test dir

