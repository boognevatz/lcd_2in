#! /bin/sh

start=$(date +%s.%N)

trap 'end=$(date +%s.%N); \
      awk -v s="$start" -v e="$end" \
      "BEGIN { printf \"Total time: %.3f ms\n\", (e - s) * 1000 }"; \
      exit' INT

curl -o streamc.raw -s -w "%{time_total}\n" http://172.16.1.1/streamc


