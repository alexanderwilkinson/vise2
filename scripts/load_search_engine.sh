#!/bin/sh
# Script to load a search engine
# Author: Abhishek Dutta <adutta@robots.ox.ac.uk
# Date: 2 July 2018

if [ "$#" -ne 3 ] ; then
  echo "${0} SEARCH_ENGINE_NAME BACKEND_PORT FRONTEND_PORT"
  killall screen
  exit
fi

NAME=$1
VERSION=1

if [ -z "$2" ]; then
  BACKEND_PORT=${2}
else
  BACKEND_PORT=10001
fi
if [ -z "$3" ]; then
  FRONTEND_PORT=${3}
else
  FRONTEND_PORT=10002
fi

REPO_DIR="/home/tlm/mydata/vise/repo"

if screen -list | grep -q "${NAME}_backend"; then
  echo "Closing existing ${NAME} backend ..."
  screen -X -S "${NAME}_backend" quit 
fi

echo "Loading backend for "$NAME" in port ${BACKEND_PORT}"
screen -dmS "${NAME}_backend" sh -c "LD_LIBRARY_PATH=\"/home/tlm/deps/vise/lib/lib\" /home/tlm/dev/vise_stable/vise/bin/api_v2 ${BACKEND_PORT} ${NAME} \"${REPO_DIR}/${NAME}/${VERSION}/vise_data/config.txt\""

if screen -list | grep -q "${NAME}_frontend"; then
  echo "Closing existing ${NAME} frontend ..."
  screen -X -S "${NAME}_frontend" quit 
fi

echo "Loading frontend for "$NAME" in port ${FRONTEND_PORT}"
screen -dmS "${NAME}_frontend" sh -c "python /home/tlm/dev/vise_stable/vise/src/search_engine/relja_retrival/src/ui/web/webserver.py ${FRONTEND_PORT} ${NAME} ${BACKEND_PORT} 0 \"${REPO_DIR}/${NAME}/${VERSION}/vise_data/config.txt\""

echo "done"

