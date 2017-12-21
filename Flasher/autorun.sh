#!/bin/sh
echo timer > /sys/class/leds/beaglebone\:green\:usr0/trigger 

#un-comment the following line to perform a backup
#dd if=/dev/mmcblk1 bs=16M | gzip -c > /mnt/BBB-eMMC-$RANDOM.img.gz

#un-comment the following 6 lines to perform a restore (be sure to replace XXXXX with your image name)
gunzip -c /mnt/BBB-eMMC-10052.img.gz | dd of=/dev/mmcblk1 bs=16M
UUID=$(/sbin/blkid -c /dev/null -s UUID -o value /dev/mmcblk1p2)
mkdir -p /mnt
mount /dev/mmcblk1p2 /mnt
sed -i "s/^uuid=.*\$/uuid=$UUID/" /mnt/boot/uEnv.txt
umount /mnt

sync
echo default-on > /sys/class/leds/beaglebone\:green\:usr0/trigger
