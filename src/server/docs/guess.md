# guesses about format

## setup
```sh
wsl hostname -I
ssh -J elliothb@172.24.173.47 root@172.16.1.100 -Y
```
```ps1
$env:DISPLAY = "localhost:0"
```

## values
common values

### fourcc
* KSXP
* MRON
* PPA7
* kxfp

* QdiN
* UdiN
* KdiN
* PWdiN

## ipaddrs

the reference frame is racnoded1

```
127.0.0.1       localhost.localdomain   localhost

172.16.1.100    racnoded1.elliothb.com  racnoded1

192.168.17.100  racnoded1-priv.elliothb.com     racnoded1-priv

172.16.1.130    racnoded1-vip.elliothb.com      racnoded1-vip

172.16.1.164    racnodedc1-cman.elliothb.com    racnodedc1-cman

172.16.1.101    racnoded2.elliothb.com  racnoded2

192.168.17.101  racnoded2-priv.elliothb.com     racnoded2-priv

172.16.1.131    racnoded2-vip.elliothb.com      racnoded2-vip
```

```
bash-4.2# ip addr
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host
       valid_lft forever preferred_lft forever
26: eth0@if27: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default
    link/ether 02:42:c0:a8:11:64 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 192.168.17.100/24 brd 192.168.17.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet 169.254.1.169/19 brd 169.254.31.255 scope global eth0:1
       valid_lft forever preferred_lft forever
28: eth1@if29: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default
    link/ether 02:42:ac:10:01:64 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 172.16.1.100/24 brd 172.16.1.255 scope global eth1
       valid_lft forever preferred_lft forever
    inet 172.16.1.231/24 brd 172.16.1.255 scope global secondary eth1:1
       valid_lft forever preferred_lft forever
    inet 172.16.1.232/24 brd 172.16.1.255 scope global secondary eth1:3
       valid_lft forever preferred_lft forever
    inet 172.16.1.130/24 brd 172.16.1.255 scope global secondary eth1:5
       valid_lft forever preferred_lft forever
```

## ports
ports of interest

[rac-startup](https://doyensys.com/blogs/oracle-rac-startup-sequence/)

### 111 - sunrpc
not sure what uses this

### 1521 - ora (listed as ncube-lm in netstat)
orcl external listener

### 1525 - orasrv (listed as prospero-np in netstat)
orcl connection manager. communication between racnodedc1-cman and racnoded(1/2)
for managing incoming connections.

### 6100 - oraagent
seems to be http traffic

### 6100 - ipv6 ons
alot of open sockets on 6100 but very little traffic, i guess
its not enabled yet.

### 6200 - ipv4 ons
encrypted with tls, this is the notification service.
why is this encrypted when nothing else is?

### 21455 - ologgerd
encrypted with tls, cluster logger service for communicating with osysmond
[docs](https://docs.oracle.com/en/database/oracle/oracle-database/19/atnms/understanding-cluster-health-monitor-services.html)

## communication

rpc talking is done over the link local interface eth0:1 mostly.

ora_lreg_orclcd seems to provide a keepalive tcp socket
[ora_lreg_orclcd](https://docs.oracle.com/en/database/oracle/oracle-database/18/refrn/background-processes.html) according to the docs its for communicating with listeners.

## packets

### packet1
sent from 169.254.16.216:9294 to 169.254.1.169:63675
packet1 is a DCE/RPC packet, seems oracle uses it for some stuff

### packet2
from 169.254.16.216:43863 to 169.254.1.169:56544
looks like a query, i think the placeholder values are first and then the query text after that.
lots of padding for some reason

### packet3
Src: 169.254.1.169, Dst: 169.254.16.216
Src Port: 29292, Dst Port: 26932
    Length: 3480
    UDP payload (3472 bytes)

sent just before packet2 and contains mostly the same content
maybe a response to a query includes the query body?

### RACNODED1
```sh
bash-4.2# ss -np -A tcp,udp
Netid State      Recv-Q Send-Q                   Local Address:Port                                  Peer Address:Port
tcp   ESTAB      0      0                        169.254.0.206:26127                                169.254.1.112:11582               users:(("tnslsnr",pid=9783,fd=19))
tcp   ESTAB      0      0                        169.254.0.206:63021                                169.254.0.206:21455               users:(("java",pid=9302,fd=76))
tcp   ESTAB      0      0                        169.254.0.206:21455                                169.254.0.206:39965               users:(("ologgerd",pid=8437,fd=144))
tcp   ESTAB      0      0                        169.254.0.206:26127                                169.254.26.74:24826               users:(("tnslsnr",pid=9783,fd=18))
tcp   ESTAB      0      0                            127.0.0.1:50881                                    127.0.0.1:58385               users:(("ocssd.bin",pid=2649,fd=236))
tcp   ESTAB      0      0                       192.168.17.100:46368                               192.168.17.102:1525                users:(("asm_lreg_+asm1",pid=8420,fd=60))
tcp   ESTAB      0      0                         172.16.1.100:22                                    172.16.1.102:63856               users:(("sshd",pid=448793,fd=3))
tcp   ESTAB      0      0                       192.168.17.100:65274                               192.168.17.101:1525                users:(("asm_lreg_+asm1",pid=8420,fd=58))
tcp   ESTAB      0      0                         172.16.1.100:26022                                 172.16.1.130:1521                users:(("asm_lreg_+asm1",pid=8420,fd=53))
tcp   ESTAB      0      0                         172.16.1.130:1521                                  172.16.1.164:48828               users:(("oracle_243494_o",pid=243494,fd=20))
tcp   ESTAB      0      0                            127.0.0.1:58385                                    127.0.0.1:50881               users:(("ocssd.bin",pid=2649,fd=237))
tcp   ESTAB      0      0                       192.168.17.100:57728                               192.168.17.100:1525                users:(("asm_lreg_+asm1",pid=8420,fd=51))
tcp   ESTAB      0      0                         172.16.1.100:22                                    172.16.1.102:21772               users:(("sshd",pid=263773,fd=3))
tcp   ESTAB      0      0                        169.254.0.206:21455                                169.254.1.112:52847               users:(("ologgerd",pid=8437,fd=162))
tcp   ESTAB      0      0                         172.16.1.232:1521                                  172.16.1.100:44188               users:(("tnslsnr",pid=9783,fd=17))
tcp   ESTAB      0      0                       192.168.17.100:57738                               192.168.17.100:1525                users:(("asm_lreg_+asm1",pid=8420,fd=52))
tcp   ESTAB      0      0                         172.16.1.100:16376                                 172.16.1.102:6200                users:(("ons",pid=10367,fd=11))
tcp   ESTAB      0      0                        169.254.0.206:21455                                169.254.26.74:43123               users:(("ologgerd",pid=8437,fd=153))
tcp   ESTAB      0      0                         172.16.1.130:1521                                  172.16.1.164:43838               users:(("oracle_242162_o",pid=242162,fd=20))
tcp   ESTAB      0      0                         172.16.1.130:1521                                  172.16.1.100:63144               users:(("tnslsnr",pid=9729,fd=18))
tcp   ESTAB      0      0                         172.16.1.100:44188                                 172.16.1.232:1521                users:(("ora_lreg_orclcd",pid=10718,fd=54))
tcp   ESTAB      0      0                       192.168.17.100:1525                                192.168.17.100:57738               users:(("tnslsnr",pid=9246,fd=16))
tcp   ESTAB      0      0                       192.168.17.100:1525                                192.168.17.100:57728               users:(("tnslsnr",pid=9246,fd=15))
tcp   ESTAB      0      0                        169.254.0.206:39965                                169.254.0.206:21455               users:(("osysmond.bin",pid=2598,fd=185))
tcp   ESTAB      0      0                        169.254.0.206:21455                                169.254.0.206:63021               users:(("ologgerd",pid=8437,fd=150))
tcp   ESTAB      0      0                       192.168.17.100:1525                                192.168.17.101:53216               users:(("tnslsnr",pid=9246,fd=18))
tcp   ESTAB      0      0                        169.254.0.206:10684                                169.254.26.74:10701               users:(("ora_lreg_orclcd",pid=10718,fd=51))
tcp   ESTAB      0      0                         172.16.1.100:63144                                 172.16.1.130:1521                users:(("ora_lreg_orclcd",pid=10718,fd=59))
tcp   ESTAB      0      0                       192.168.17.100:1525                                192.168.17.102:59926               users:(("tnslsnr",pid=9246,fd=19))
tcp   ESTAB      0      0                         172.16.1.100:33482                                 172.16.1.164:1521                users:(("ora_lreg_orclcd",pid=10718,fd=50))
tcp   ESTAB      0      0                        169.254.0.206:38930                                169.254.1.112:26753               users:(("ora_lreg_orclcd",pid=10718,fd=52))
tcp   ESTAB      0      0                         172.16.1.130:1521                                  172.16.1.100:26022               users:(("tnslsnr",pid=9729,fd=21))
tcp   ESTAB      0      0                                [::1]:29454                                        [::1]:6100                users:(("tnslsnr",pid=9783,fd=16))
tcp   ESTAB      0      0                                [::1]:20294                                        [::1]:6100                users:(("tnslsnr",pid=9246,fd=17))
tcp   ESTAB      0      0                                [::1]:6100                                         [::1]:20294               users:(("ons",pid=10367,fd=14))
tcp   ESTAB      0      0                                [::1]:6100                                         [::1]:29444               users:(("ons",pid=10367,fd=9))
tcp   ESTAB      0      0                                [::1]:29444                                        [::1]:6100                users:(("tnslsnr",pid=9729,fd=17))
tcp   ESTAB      0      0                [::ffff:172.16.1.100]:6200                         [::ffff:172.16.1.101]:18834               users:(("ons",pid=10367,fd=12))
tcp   ESTAB      0      0                                [::1]:20284                                        [::1]:6100                users:(("oraagent.bin",pid=8783,fd=221))
tcp   ESTAB      0      0                                [::1]:6100                                         [::1]:20284               users:(("ons",pid=10367,fd=13))
tcp   ESTAB      0      0                                [::1]:6100                                         [::1]:29454               users:(("ons",pid=10367,fd=10))
```

```sh
bash-4.2# readlink /proc/8437/exe # ologgerd
/u01/app/19.3.0/grid/bin/ologgerd
bash-4.2# readlink /proc/2598/exe # osysmond.bin
/u01/app/19.3.0/grid/bin/osysmond.bin
bash-4.2# readlink /proc/8783/exe # oraagent.bin
/u01/app/19.3.0/grid/bin/oraagent.bin
bash-4.2# readlink /proc/10367/exe # ons
/u01/app/19.3.0/grid/opmn/bin/ons
bash-4.2# readlink /proc/3769/exe # tnslsnr
/u01/app/19.3.0/grid/bin/tnslsnr
```

### RACNODED3
```sh
bash-4.2# ss -np -A tcp,udp
Netid State      Recv-Q Send-Q             Local Address:Port                            Peer Address:Port
tcp   ESTAB      0      0                      127.0.0.1:12429                              127.0.0.1:62005               users:(("ocssd.bin",pid=2677,fd=235))
tcp   ESTAB      0      0                  169.254.26.74:43123                          169.254.0.206:21455               users:(("osysmond.bin",pid=2626,fd=181))
tcp   ESTAB      0      0                   172.16.1.102:22                                172.16.1.1:44418               users:(("sshd",pid=73876,fd=3))
tcp   ESTAB      0      0                   172.16.1.102:44428                           172.16.1.230:1521                users:(("ora_lreg_orclcd",pid=7493,fd=52))
tcp   ESTAB      0      0                   172.16.1.132:1521                            172.16.1.102:54174               users:(("tnslsnr",pid=4293,fd=19))
tcp   ESTAB      0      0                  169.254.26.74:42582                          169.254.1.112:26753               users:(("ora_lreg_orclcd",pid=7493,fd=54))
tcp   ESTAB      0      0                 192.168.17.102:62844                         192.168.17.102:1525                users:(("asm_lreg_+asm3",pid=7153,fd=52))
tcp   ESTAB      0      0                   172.16.1.102:54174                           172.16.1.132:1521                users:(("ora_lreg_orclcd",pid=7493,fd=59))
tcp   ESTAB      0      0                 192.168.17.102:59926                         192.168.17.100:1525                users:(("asm_lreg_+asm3",pid=7153,fd=58))
tcp   ESTAB      0      0                  169.254.26.74:10701                          169.254.0.206:10684               users:(("tnslsnr",pid=3769,fd=17))
tcp   ESTAB      0      0                 192.168.17.102:41750                         192.168.17.101:1525                users:(("asm_lreg_+asm3",pid=7153,fd=53))
tcp   ESTAB      0      0                      127.0.0.1:62005                              127.0.0.1:12429               users:(("ocssd.bin",pid=2677,fd=236))
tcp   ESTAB      0      0                   172.16.1.230:1521                            172.16.1.102:44428               users:(("tnslsnr",pid=3769,fd=18))
tcp   ESTAB      0      0                   172.16.1.132:1521                            172.16.1.102:54188               users:(("tnslsnr",pid=4293,fd=18))
tcp   ESTAB      0      0                 192.168.17.102:1525                          192.168.17.101:62042               users:(("tnslsnr",pid=5995,fd=17))
tcp   ESTAB      0      0                 192.168.17.102:1525                          192.168.17.102:62854               users:(("tnslsnr",pid=5995,fd=19))
tcp   ESTAB      0      0                 192.168.17.102:1525                          192.168.17.100:46368               users:(("tnslsnr",pid=5995,fd=16))
tcp   ESTAB      0      0                   172.16.1.102:54188                           172.16.1.132:1521                users:(("asm_lreg_+asm3",pid=7153,fd=51))
tcp   ESTAB      0      0                   172.16.1.102:50830                           172.16.1.164:1521                users:(("ora_lreg_orclcd",pid=7493,fd=50))
tcp   ESTAB      0      0                 192.168.17.102:1525                          192.168.17.102:62844               users:(("tnslsnr",pid=5995,fd=18))
tcp   ESTAB      0      0                  169.254.26.74:24826                          169.254.0.206:26127               users:(("ora_lreg_orclcd",pid=7493,fd=51))
tcp   ESTAB      0      0                   172.16.1.102:21772                           172.16.1.100:22                  users:(("ssh",pid=211895,fd=3))
tcp   ESTAB      0      0                 192.168.17.102:62854                         192.168.17.102:1525                users:(("asm_lreg_+asm3",pid=7153,fd=60))
tcp   ESTAB      0      0                  169.254.26.74:10701                          169.254.1.112:25380               users:(("tnslsnr",pid=3769,fd=19))
tcp   ESTAB      0      0                   172.16.1.102:33966                           172.16.1.101:6200                users:(("ons",pid=3405,fd=11))
tcp   ESTAB      0      0                          [::1]:6100                                   [::1]:44996               users:(("ons",pid=3405,fd=13))
tcp   ESTAB      0      0                          [::1]:44938                                  [::1]:6100                users:(("tnslsnr",pid=3769,fd=13))
tcp   ESTAB      0      0                          [::1]:44984                                  [::1]:6100                users:(("oraagent.bin",pid=3333,fd=179))
tcp   ESTAB      0      0                          [::1]:27588                                  [::1]:6100                users:(("tnslsnr",pid=5995,fd=13))
tcp   ESTAB      0      0          [::ffff:172.16.1.102]:6200                   [::ffff:172.16.1.100]:16376               users:(("ons",pid=3405,fd=9))
tcp   ESTAB      0      0                          [::1]:44996                                  [::1]:6100                users:(("tnslsnr",pid=4293,fd=13))
tcp   ESTAB      0      0          [::ffff:172.16.1.102]:9152                   [::ffff:172.16.1.101]:5000                users:(("java",pid=959,fd=66))
tcp   ESTAB      0      0                          [::1]:6100                                   [::1]:44984               users:(("ons",pid=3405,fd=12))
tcp   ESTAB      0      0          [::ffff:172.16.1.102]:63428                  [::ffff:172.16.1.101]:5000                users:(("java",pid=959,fd=60))
tcp   ESTAB      0      0                          [::1]:6100                                   [::1]:44938               users:(("ons",pid=3405,fd=10))
tcp   ESTAB      0      0                          [::1]:6100                                   [::1]:27588               users:(("ons",pid=3405,fd=14))
```

## replacing /u01/app/19.3.0/grid/opmn/bin/ons
```sh
/u01/app/19.3.0/grid/bin/srvctl stop ons
sudo cp /opt/shared/orarpc-test /u01/app/19.3.0/grid/opmn/bin/ons
/u01/app/19.3.0/grid/bin/srvctl start ons
```

```log
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:18.981] Net: WSAStartup  : SUCCESS
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:18.981] Net: VERSION     : 1.0
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:18.981] Net: DESCRIPTION : `POSIX SOCKETS`
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:18.981] Net: STATUS      : `ACTIVE`
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:18.981] Launch: args = [/u01/app/19.3.0/grid/opmn/bin/ons, -a, ping]
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)INVALID SYNTAX: Unexpected value (-a).
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)INVALID SYNTAX: Unexpected value (ping).
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:19.006] Net: WSAStartup  : SUCCESS
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:19.006] Net: VERSION     : 1.0
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:19.006] Net: DESCRIPTION : `POSIX SOCKETS`
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:19.006] Net: STATUS      : `ACTIVE`
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)[INFO][21:48:19.006] Launch: args = [/u01/app/19.3.0/grid/opmn/bin/ons, -d]
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)INVALID SYNTAX: Unexpected value (-d).
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)onsctl start: ons failed to start
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)
2024-11-22 03:31:59.405 :CLSDYNAM:413181696: [ ora.ons]{1:11962:5476} [start] (:CLSN00010:)Utils:execCmd scls_process_join() uret 1
```