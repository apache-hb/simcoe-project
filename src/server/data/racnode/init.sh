#!/bin/bash

touch /var/lock/subsys/local
/opt/scripts/setup.sh
nohup /opt/scripts/startup/runOracle.sh &
