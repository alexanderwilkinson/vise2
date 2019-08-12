#!/bin/sh

if ! screen -list | grep -q "ox5k_backend"; then
  echo "Starting ox5k backend ..."
  screen -dmS ox5k_backend ./vggdebug1_start_ox5k_backend.sh
else
  echo "ox5k backend demo already started!"
fi

if ! screen -list | grep -q "ox5k_frontend"; then
  echo "Starting ox5k frontend ..."
  screen -dmS ox5k_frontend ./vggdebug1_start_ox5k_frontend.sh
else
  echo "ox5k frontend demo already started!"
fi

