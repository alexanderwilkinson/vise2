#!/bin/sh
source /ssd/adutta/dep/python/virtualenv/19c_image_match/bin/activate
export PATH=$PATH:/ssd/adutta/dep/common/lib/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/ssd/adutta/dep/common/lib/lib

HOSTFILE="./tmp_hostfile.txt"
LOGFILE="./vise.log"
NUM_PROC=32
echo "localhost slots=32" > $HOSTFILE
echo "VISE indexing started" > $LOGFILE

# cleanup temp folder
rm -fr /data/seebibyte_tap/19c_image_match/vise/19c_image_match/1/temp/
mkdir /data/seebibyte_tap/19c_image_match/vise/19c_image_match/1/temp/

date >> $LOGFILE
mpirun --hostfile $HOSTFILE -np $NUM_PROC /ssd/adutta/dev/vise/bin/compute_index_v2 trainDescs 19c_image_match /data/seebibyte_tap/19c_image_match/vise/19c_image_match/1/vise_data/config.txt >> $LOGFILE
date >> $LOGFILE
mpirun --hostfile $HOSTFILE -np $NUM_PROC python /ssd/adutta/dev/vise/src/search_engine/relja_retrival/src/v2/indexing/compute_clusters.py 19c_image_match /data/seebibyte_tap/19c_image_match/vise/19c_image_match/1/vise_data/config.txt 30 >> $LOGFILE
date >> $LOGFILE
mpirun --hostfile $HOSTFILE -np $NUM_PROC /ssd/adutta/dev/vise/bin/compute_index_v2 trainAssign 19c_image_match /data/seebibyte_tap/19c_image_match/vise/19c_image_match/1/vise_data/config.txt >> $LOGFILE
date >> $LOGFILE
mpirun --hostfile $HOSTFILE -np $NUM_PROC /ssd/adutta/dev/vise/bin/compute_index_v2 trainHamm 19c_image_match /data/seebibyte_tap/19c_image_match/vise/19c_image_match/1/vise_data/config.txt >> $LOGFILE
date >> $LOGFILE
mpirun --hostfile $HOSTFILE -np $NUM_PROC /ssd/adutta/dev/vise/bin/compute_index_v2 index 19c_image_match /data/seebibyte_tap/19c_image_match/vise/19c_image_match/1/vise_data/config.txt >> $LOGFILE
date >> $LOGFILE
echo "Finished" >> $LOGFILE

rm $HOSTFILE

