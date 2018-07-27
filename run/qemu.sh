#!/bin/bash

BASEDIR=`dirname $0`
cd $BASEDIR

./create_image.sh

qemu-system-x86_64 -smp 2 -m 128 -monitor stdio -hda disk.img
