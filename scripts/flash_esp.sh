#!/bin/bash

PORT=$1
BOOTLOADER_FILE=$2
PARTITION_FILE=$3
IMAGE_FILE=$4

sh -c "esptool.py \
      --chip esp32 \
      --port ${PORT} \
      --baud 460800 \
      --before default-reset write-flash \
      -z \
      --flash-mode dout \
      --flash-freq 40m \
      --flash-size detect \
      0x1000 ${BOOTLOADER_FILE} \
      0x8000 ${PARTITION_FILE} \
      0x10000 ${IMAGE_FILE}"
