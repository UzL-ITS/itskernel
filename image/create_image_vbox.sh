#!/bin/bash

# Creates a VirtualBox disk image.

echo "Creating disk image..."
./create_image.sh

echo "Converting disk image to VirtualBox VDI format..."
rm disk.vdi
VBoxManage convertdd disk.img disk.vdi --format VDI