#!/bin/sh

cd /ssd/adutta/dev/vise/bin
LD_LIBRARY_PATH="/lib64/openmpi/lib/:/home/tlm/deps/vise/lib/lib" ./api_v2 10001 ox5k /ssd/adutta/mydata/vise/repo/ox5k/1/vise_data/config.txt

