#!/bin/sh

# Base image present?
echo "Looking for base image..."
if [ ! -f disk.img ]; then
	if [ ! -f disk.img.xz ]; then
	
		# Prepare the base GRUB disk image disk.img.xz
		# Partially copied from https://github.com/grahamedgecombe/arc/README.markdown
		echo "Base image not found, creating new one."

		echo "Mount new file disk.img on loop0..."
		dd if=/dev/zero of=disk.img bs=512 count=32130
		sudo losetup /dev/loop0 disk.img

		echo "Run fdisk to partition disk.img..."
		cat fdisk.in | sudo fdisk /dev/loop0

		echo "Mount disk.img partition..."
		sudo losetup -o 1048576 /dev/loop1 /dev/loop0

		echo "Format and mount partition file system..."
		sudo mke2fs /dev/loop1
		mkdir temp
		sudo mount /dev/loop1 temp

		echo "Install GRUB on partition (there might occur errors which can be ignored)..."
		sudo grub-install --target=i386-pc --boot-directory=temp/boot --modules="biosdisk part_msdos ext2 vga vbe" /dev/loop0

		echo "Unmount and cleanup..."
		sudo umount temp
		sudo losetup -d /dev/loop1
		sudo losetup -d /dev/loop0
		rmdir temp

		xz -9 disk.img

	fi
	
	echo "Extracting base image..."
	xzcat disk.img.xz > disk.img
fi

echo "Mounting base disk image..."
mkdir -p mnt
sudo mount -o loop,offset=1048576 disk.img mnt

echo "Copying files..."
sudo mkdir -p mnt/bin
sudo cp ../code/kernel.elf mnt/boot
sudo cp ../code/ui.elf mnt/bin
sudo cp grub.cfg mnt/boot/grub
sync

echo "Unmounting disk image..."
sudo umount mnt
rm -d mnt

echo "Done."