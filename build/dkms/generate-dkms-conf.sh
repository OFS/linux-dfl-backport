#!/bin/sh
# Copyright(c) 2022, Intel Corporation

src=$1
conf=$2

generate() {
  local i=0
  for m in $(make -f $src/Makefile dkms); do
    echo BUILT_MODULE_NAME[$i]=\"$m\"
    echo DEST_MODULE_LOCATION[$i]=\"/kernel/drivers\"
    i=$((i + 1))
  done
}

generate > $conf
