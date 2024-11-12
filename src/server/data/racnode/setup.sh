#!/bin/bash

# disable getty@tty1 service, we dont need it and its broken
systemctl stop getty@tty1
systemctl disable getty@tty1

# disable chronyd service, systemd is all kinds of broken
# inside docker. we'll start chrony ourselves
systemctl stop chronyd
systemctl disable chronyd

systemctl reset-failed

/usr/sbin/chronyd

# rename the symvers file to include the kernel version
RELEASE=$(uname -r)
cp /boot/symvers.gz /usr/lib/modules/symvers-$RELEASE.gz
