mkdir -p container/dev
mkdir -p container/dev/oracleoci
mkdir -p /dev/oracleoci

if [ ! -e "container/dev/oracleoci/oraclevdd" ]; then
    dd if=/dev/zero of=container/dev/oracleoci/oraclevdd bs=1M count=1024
fi

if [ ! -e "container/dev/oracleoci/oraclevde" ]; then
    dd if=/dev/zero of=container/dev/oracleoci/oraclevde bs=1M count=1024
fi

losetup -f container/dev/oracleoci/oraclevdd
losetup -f container/dev/oracleoci/oraclevde

# get loop device associated with the file and symlink it to /dev/oracleoci
LOOPDEVDD=$(losetup -a | grep oraclevdd | awk '{print $1}')
LOOPDEVDE=$(losetup -a | grep oraclevde | awk '{print $1}')

ln -s $LOOPDEVDD /dev/oracleoci/oraclevdd
ln -s $LOOPDEVDE /dev/oracleoci/oraclevde
