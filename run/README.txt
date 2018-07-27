Create disk.img.xz disk template with pre installed GRUB:
./prepare_grub_image.sh

Write kernel to disk template and create bootable image:
./create_image.sh

Install disk image to USB stick on /dev/sdb:
sudo dd if=disk.img of=/dev/sdb