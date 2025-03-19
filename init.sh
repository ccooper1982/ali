#!/bin/bash

loadkeys uk

echo Increasing size of /tmp to 3GiB
mount -o remount,size=3g tmpfs /tmp
if [ $? -ne 0 ]; then
  exit 1
fi

echo Creating /tmp/ali
mkdir /tmp/ali
if [ $? -ne 0 ]; then
  exit 1
fi

echo Mounting /tmp/ali to /root/ali
mount --mkdir -t tmpfs /tmp/ali /root/ali
if [ $? -ne 0 ]; then
  exit 1
fi

echo Strapping pacman onto /root/ali
pacstrap -K /root/ali base qt6-base libxcb virtualbox-guest-utils
if [ $? -ne 0 ]; then
  exit 1
fi

modprobe -a vboxguest vboxsf vboxvideo

#arch-chroot /root/mnt/ali
echo Updating ldd search path
echo /usr/bin >> /etc/ld.so.conf

echo Done
