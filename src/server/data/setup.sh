mkdir -p container
mkdir -p container/.secrets
mkdir -p container/asm

touch container/rac_host_file
touch container/.secrets/pwd.key
touch container/.secrets/common_os_pwdfile

openssl rand -out container/.secrets/pwd.key -hex 64

openssl enc -aes-256-cbc -salt \
    -in container/.secrets/common_os_pwdfile \
    -out container/.secrets/common_os_pwdfile.enc \
    -pass file:container/.secrets/pwd.key

rm -f container/.secrets/common_os_pwdfile
