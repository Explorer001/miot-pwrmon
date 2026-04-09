#!/bin/bash

PORT=$1
MODE=$2
BOOTLOADER_FILE=$3
PARTITION_FILE=$4
IMAGE_FILE=$5

SPLITTER=""

if [ $MODE = "NEW" ]; then
      SPLITTER="-"
elif [ $MODE = "LEGACY" ]; then
      SPLITTER="_"
else
      echo "Mode needs to be NEW or LEGACY"
      exit 1
fi

sh -c "esptool.py \
      --chip esp32 \
      --port ${PORT} \
      --baud 460800 \
      --before default${SPLITTER}reset write${SPLITTER}flash \
      -z \
      --flash${SPLITTER}mode dout \
      --flash${SPLITTER}freq 40m \
      --flash${SPLITTER}size detect \
      0x1000 ${BOOTLOADER_FILE} \
      0x8000 ${PARTITION_FILE} \
      0x10000 ${IMAGE_FILE}"
