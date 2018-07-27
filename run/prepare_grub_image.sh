#!/bin/bash

# From https://github.com/grahamedgecombe/arc/README.markdown

echo "### Mount disk.img on loop0 ###"
dd if=/dev/zero of=disk.img bs=512 count=32130
sudo losetup /dev/loop0 disk.img

echo "### Run fdisk to partition disk.img ###"
cat fdisk.in | sudo fdisk /dev/loop0

echo "### Mount disk.img partition ###"
sudo losetup -o 1048576 /dev/loop1 /dev/loop0

echo "### Format and mount partition file system ###"
sudo mke2fs /dev/loop1
mkdir temp
sudo mount /dev/loop1 temp

echo "### Install GRUB on partition ###"
sudo grub-install --target=i386-pc --boot-directory=temp/boot --modules="biosdisk part_msdos ext2 vga vbe" /dev/loop0

echo "### Unmount and cleanup ###"
sudo umount temp
sudo losetup -d /dev/loop1
sudo losetup -d /dev/loop0
rmdir temp

xz -9 disk.img