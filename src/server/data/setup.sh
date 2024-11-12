#!/bin/bash

mkdir -p container
mkdir -p container/secrets
mkdir -p container/asm

touch container/rac_host_file
touch container/secrets/pwd.key
touch container/secrets/common_os_pwdfile

openssl rand -out container/secrets/pwd.key -hex 64

openssl enc -aes-256-cbc -salt \
    -in password.txt \
    -out container/secrets/common_os_pwdfile.enc \
    -pass file:container/secrets/pwd.key

rm -f container/secrets/common_os_pwdfile

sudo chmod -R 777 racnode/*.sh
sudo chown -R : racnode/*.sh

cp racnode/boot/symvers-5.15.0-301.163.5.2.el8uek.x86_64.gz \
    racnode/boot/symvers-$(uname -r).gz

cp racnode/boot/symvers-5.15.0-301.163.5.2.el8uek.x86_64.gz \
    racnode/boot/symvers-$(uname -r).x86_64.gz
