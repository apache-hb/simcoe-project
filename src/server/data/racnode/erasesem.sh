#!/bin/bash

# stop the hanged instance
$GRID_HOME/bin/srvctl stop instance -db $ORACLE_SID -node $HOSTNAME

# kill the crsd process so it will release the hanged database instance.
pid=$(ps -ef | grep $GRID_HOME/bin/crsd.bin | grep -v grep | awk '{print $2}')
if [ ! -z $pid ]; then
    kill -9 $pid
fi

# oracle has this strange quirk where if the database isnt shut down properly
# it will leak all of its semaphores and then wont be able to start until
# they are manually removed.
# luckily all semaphores belong to the oracle user, so we can just remove
# all of them.

$GRID_HOME/bin/srvctl stop database -d $ORACLE_SID

for i in $(ipcs -s | grep oracle | awk '{print $2}'); do
    ipcrm -m $i
done

$GRID_HOME/bin/srvctl start database -d $ORACLE_SID
