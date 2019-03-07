path %PATH%;C:\Program Files\qemu
qemu-system-x86_64 --trace events=traceevents.txt -smp 2 -m 128 -monitor stdio -hda ../disk.img -s -vga std -machine q35