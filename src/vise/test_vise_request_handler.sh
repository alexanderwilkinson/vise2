#!/bin/sh
HOSTNAME="localhost"
PORT="9973"

SEARCH_ENGINE_NAME="ox5k"
SEARCH_ENGINE_VERSION="1"
IMAGE_DATA_DIR="/home/tlm/dev/vise/data/test/ox5k"

#SEARCH_ENGINE_NAME="sciam"
#SEARCH_ENGINE_VERSION="1"
#IMAGE_DATA_DIR="/data/datasets/vise/sciam/data/jpg"

echo "Uploading images from ${IMAGE_DATA_DIR} ..."
TMP_FILENAME="/tmp/vise_tmpfile.txt"
find ${IMAGE_DATA_DIR}/*.jpg -printf "%f\n" -type f > $TMP_FILENAME
parallel --no-notice --bar -P 100 --silent -a $TMP_FILENAME curl -X POST --silent --output /dev/null --show-error --data-binary @${IMAGE_DATA_DIR}"/"{1} "${HOSTNAME}:${PORT}/vise/repo/${SEARCH_ENGINE_NAME}/${SEARCH_ENGINE_VERSION}/add_file?filename=\""{1}"\""
rm $TMP_FILENAME

echo "Running indexing"
curl -X POST "${HOSTNAME}:${PORT}/vise/repo/${SEARCH_ENGINE_NAME}/${SEARCH_ENGINE_VERSION}/index_start"




