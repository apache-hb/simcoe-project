#!/bin/bash

touch /var/lock/subsys/local
/opt/scripts/nogetty.sh
nohup /opt/scripts/startup/runOracle.sh &
