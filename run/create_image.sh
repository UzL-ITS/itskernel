#!/bin/sh
if [ ! -f disk.img ]; then
  if [ ! -f disk.img.xz ]; then
    echo "No disk image template. Run ./prepare_grub_image.sh first." >&2
    exit 1
  fi
  xzcat disk.img.xz > disk.img
fi
mkdir -p mnt
sudo mount -o loop,offset=1048576 disk.img mnt
sudo mkdir -p mnt/bin
sudo cp ../code/kernel/itskernel mnt/boot
sudo cp ../code/ui/itskernelui mnt/bin
sudo cp grub.cfg mnt/boot/grub
sync
sudo umount mnt
rm disk.vdi
VBoxManage convertdd disk.img disk.vdi --format VDI