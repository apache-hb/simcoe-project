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

## interfaces

00000000-0000-0000-a9fe-1a4a00000000

00000000-0000-0000-a9fe-017000000000

00000000-0000-0000-a9fe-00ce00000000

as far as i can tell these are the big 3 interfaces,
I cant tell much about which operations are being called on them
as oracles implementation of dce/rpc either isnt compliant or has
extensions that change the header, so wireshark isnt able to grok
much of it sensibly. interface versions and opnums are pretty mangled
in the packet headers.

I know oracledb engineers really like their enums with ascii representations
so im guessing that `Tm#`, `Mm#`, `Sm#` all mean something important. they
turn up alot in the message stream. Wild guess i'd say something to do with
data encoding, Sm# seems to be sent alot in repeating blocks so maybe arrays
have Sm# at the header of each element depending on the type?
Tm# seems to only be sent once per message so maybe its part of an internal header.

The actual packet header is very wrong, its marked as EBCDIC encoding when its
actually ascii encoded.

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
# copy daemon into shared volume
sudo cp ./build/src/plugins/orarpc/orarpc-test ./src/server/data/container/shared/orarpc-test
```

```sh
# from inside the container replace the daemon
/u01/app/19.3.0/grid/opmn/bin/onsctl stop
sudo cp /opt/shared/orarpc-test /u01/app/19.3.0/grid/opmn/bin/ons
/u01/app/19.3.0/grid/opmn/bin/onsctl start
```

```sh
# srvctl can also manage ons but is slower
/u01/app/19.3.0/grid/bin/srvctl stop ons
/u01/app/19.3.0/grid/bin/srvctl start ons

tail -f /u01/app/grid/diag/crs/racnoded1/crs/trace/crsd_oraagent_grid.trc
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

ons uses a self signed 2048 bit rsa key using cipher ECDHE-RSA-AES256-GCM-SHA384
Peer signing digest: SHA384
Peer signature type: RSA
looks as if a new key is generated every 100 years

```sh
# ons openssl tls cert
CONNECTED(00000003)
Can't use SSL_get_servername
depth=0 C = US, CN = ons_ssl
verify error:num=18:self-signed certificate
verify return:1
depth=0 C = US, CN = ons_ssl
verify return:1
40B78E8DF17F0000:error:0A000126:SSL routines:ssl3_read_n:unexpected eof while reading:../ssl/record/rec_layer_s3.c:317:
---
Certificate chain
 0 s:C = US, CN = ons_ssl
   i:C = US, CN = ons_ssl
   a:PKEY: rsaEncryption, 2048 (bit); sigalg: RSA-SHA256
   v:NotBefore: Nov 17 00:00:00 2024 GMT; NotAfter: Nov 18 00:00:00 2124 GMT
---
Server certificate
-----BEGIN CERTIFICATE-----
MIIC3TCCAcWgAwIBAgIQLYol1hHtemb8yWSwSETa+jANBgkqhkiG9w0BAQsFADAf
MQswCQYDVQQGEwJVUzEQMA4GA1UEAwwHb25zX3NzbDAgFw0yNDExMTcwMDAwMDBa
GA8yMTI0MTExODAwMDAwMFowHzELMAkGA1UEBhMCVVMxEDAOBgNVBAMMB29uc19z
c2wwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCdi4R+kQldV+o4QaX/
qxT30qMGiX6ulmqGjgty0+XBUINs0lNsBwUYTItsJZR8ZFukuzGLAx17H0Xpk9rK
3BLJAwcJ9GLnFfBdPAmIq7/wMIXCk5NNdy+VPXih/cUTvjFNmoUBJmRFG46dSsA2
ei8IWX+xGP25gUoWhGo+1HNbv9KBXQKGL+eq314KO9p/qtdTREzVi+IOV12ZphoC
uUI5R0pyGXJy1GkvCWjz2nJD4XF7h58fV6cFu0i0zqrchH/8tsjLzUnRhge6tNlC
WnIcTk7KDKJ+yU7AmjlGVFMkYUJHIjxJ9l4UH1OHr/L9qGaig7+77veg29BQ8+4q
+JbfAgMBAAGjEzARMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEB
AGZFQidcKYMJeOTvGE6s5TDK2G/whr3PWVkZiMKa2jqd4wR9mvW/oAWuB8Qy/LXD
NZcjXsqyxZYAfNbu4j+2ol21PvntjngtXQdv2aXKtIdKVh3NcZMi/9KaN3ScOnrH
4BCNT8j6QVlWMPkGVfpsKeZ8mhejAGon4MHnwzEFUP3eQZ8ps09JIEFBlsP6Drp7
qtI86nQ782jvhRQWmMhCIMeeWn5/Iwt6gLRTNci0Ovwtm2rdcsqE7NnoKkMS6A3i
UVO0W5Of2ZUDeH+y8Lln2lZxc34/wUAcDMHeffmsHKAZzoKoOAr9vajnbg8Ki+Jj
zFiHY7JoOiKU3zPJ3vwf3g0=
-----END CERTIFICATE-----
subject=C = US, CN = ons_ssl
issuer=C = US, CN = ons_ssl
---
Acceptable client certificate CA names
C = US, CN = ons_ssl
Client Certificate Types: ECDSA sign, RSA sign, DSA sign
Requested Signature Algorithms: ECDSA+SHA256:ECDSA+SHA384:ECDSA+SHA512:RSA+SHA256:RSA+SHA384:RSA+SHA512:ECDSA+SHA224:RSA+SHA224:DSA+SHA224:DSA+SHA256:DSA+SHA384:DSA+SHA512
Shared Requested Signature Algorithms: ECDSA+SHA256:ECDSA+SHA384:ECDSA+SHA512:RSA+SHA256:RSA+SHA384:RSA+SHA512:ECDSA+SHA224:RSA+SHA224:DSA+SHA224:DSA+SHA256:DSA+SHA384:DSA+SHA512
Peer signing digest: SHA384
Peer signature type: RSA
Server Temp Key: ECDH, prime256v1, 256 bits
---
SSL handshake has read 1256 bytes and written 462 bytes
Verification error: self-signed certificate
---
New, TLSv1.2, Cipher is ECDHE-RSA-AES256-GCM-SHA384
Server public key is 2048 bit
Secure Renegotiation IS supported
Compression: NONE
Expansion: NONE
No ALPN negotiated
SSL-Session:
    Protocol  : TLSv1.2
    Cipher    : ECDHE-RSA-AES256-GCM-SHA384
    Session-ID: 16F9A44AE9B33BA98B447522B8AC7B0BA73700E3998E4AE369DAF148459E0F75
    Session-ID-ctx:
    Master-Key: 5558B8FDEEA902198888771FE0CCDD4EB6BFB6D090516B4E570324F007727054B028D8DC277D86BB572365DABFAC383E
    PSK identity: None
    PSK identity hint: None
    SRP username: None
    Start Time: 1732311720
    Timeout   : 7200 (sec)
    Verify return code: 18 (self-signed certificate)
    Extended master secret: no
---
```

```sh
# local tls
elliothb@ELLIOT-SERVER:~/github/simcoe-project/src/server/data$ openssl s_client -connect localhost:6200
CONNECTED(00000003)
Can't use SSL_get_servername
depth=0 C = CA, O = Simcoe, CN = localhost
verify error:num=18:self-signed certificate
verify return:1
depth=0 C = CA, O = Simcoe, CN = localhost
verify return:1
---
Certificate chain
 0 s:C = CA, O = Simcoe, CN = localhost
   i:C = CA, O = Simcoe, CN = localhost
   a:PKEY: rsaEncryption, 2048 (bit); sigalg: RSA-SHA1
   v:NotBefore: Nov 22 20:41:33 2024 GMT; NotAfter: Nov 22 20:41:33 2025 GMT
---
Server certificate
-----BEGIN CERTIFICATE-----
MIIC2DCCAcACAQEwDQYJKoZIhvcNAQEFBQAwMjELMAkGA1UEBhMCQ0ExDzANBgNV
BAoMBlNpbWNvZTESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTI0MTEyMjIwNDEzM1oX
DTI1MTEyMjIwNDEzM1owMjELMAkGA1UEBhMCQ0ExDzANBgNVBAoMBlNpbWNvZTES
MBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC
AQEA1VRNnxI6tIFAi+s7qvv9e/fMXAszA015AF/mDnXALwFtIS69c8YkqAEMzc2m
AOJpmM9F9+UnaowqtwRaJcd3gg1PeBQ890XP7FlzvCjfhTLu2manY8i7A6PqaPgQ
1dkj1bqkmYs7gxgDc97RL6iCFNIqASmywZKAdgbI8L7sveq3mAqBaizMpnhCMIdR
OvBmrCy4G4RhwkainrQ04tYIrYkbgEiKpWE5AUK7bO6QdDzuw/YiVwEJRfixLAyB
fAnFW+4utK7buwRaFNpZefbg2mflniYVjDBocN/fySRMrnn6XMGpdiiFSbOFAbTe
FrwueGyyeb5ZplEDcXApsDhU9wIDAQABMA0GCSqGSIb3DQEBBQUAA4IBAQBT3vAt
V9TqrQ1sIz/i2KoL1FEDN33gbwtNxPNhjMpLizGBT+ILN6lm8jvXxSLoDzYhGpQN
yM/5zJh8ER41CZ4Odgp3m4CxbDW7l/4OUR7uCTw99rwT4qssVBJ3d4wzyhDKqw2f
Flnts5tNlM3+aCdoUJ0WU1beDXeuhAyPwQdPkVmEu68N/9WiC1rmXe5BcOG9AK3G
pHNBUT+GywhVEPF3NpquvBDI5MKQHp5QIVCCCyq5k6xb1RaIqjvJfQcdDcMx2Yup
BmLg4zRjLXR+72xoAbWBlo4IqbtwjKBGFYGpA9QQzaSe+tQRwJkypdpCwa1TXT7z
7ULXWSc391dJaJFw
-----END CERTIFICATE-----
subject=C = CA, O = Simcoe, CN = localhost
issuer=C = CA, O = Simcoe, CN = localhost
---
No client certificate CA names sent
Peer signing digest: SHA256
Peer signature type: RSA-PSS
Server Temp Key: X25519, 253 bits
---
SSL handshake has read 1288 bytes and written 373 bytes
Verification error: self-signed certificate
---
New, TLSv1.3, Cipher is TLS_AES_256_GCM_SHA384
Server public key is 2048 bit
Secure Renegotiation IS NOT supported
Compression: NONE
Expansion: NONE
No ALPN negotiated
Early data was not sent
Verify return code: 18 (self-signed certificate)
---
---
Post-Handshake New Session Ticket arrived:
SSL-Session:
    Protocol  : TLSv1.3
    Cipher    : TLS_AES_256_GCM_SHA384
    Session-ID: E4F73A57F710190C7C67A19C6BB910AA278D2B5D06EFA68B0ED23EE1B8C7660A
    Session-ID-ctx:
    Resumption PSK: C19828831149B0261402EC70D81A682EEE504BA734D1F3722244C95595C6EBC6BE9288215A3B0FFAC430AAC23AF3741D
    PSK identity: None
    PSK identity hint: None
    SRP username: None
    TLS session ticket lifetime hint: 7200 (seconds)
    TLS session ticket:
    0000 - 59 cd ed 9f 8c bc be 42-12 5e 03 99 7f a8 a9 b3   Y......B.^......
    0010 - 2b cb 22 38 37 56 98 ab-08 9a e1 9a d7 9d 9e 5b   +."87V.........[
    0020 - 9e 3d e3 26 27 00 e2 68-b5 5f f8 f2 52 d4 b2 43   .=.&'..h._..R..C
    0030 - 7d 43 1a 02 74 fa 13 9a-90 e4 b9 23 94 7f d0 b5   }C..t......#....
    0040 - a4 d2 37 7d 78 4b 46 e7-97 bf 7b 39 d4 40 a0 1c   ..7}xKF...{9.@..
    0050 - eb f5 03 40 ae 20 ac dc-06 52 25 96 a3 26 bf fd   ...@. ...R%..&..
    0060 - 47 7f 06 dd ed 65 5c 2d-d3 94 f8 55 2f 14 d9 1d   G....e\-...U/...
    0070 - a7 c6 7d d9 24 1d a0 8a-c3 89 d2 8e c8 d6 7c ca   ..}.$.........|.
    0080 - bf 04 6c 99 76 7b 58 9a-5d 05 02 77 c4 a0 aa 1d   ..l.v{X.]..w....
    0090 - f1 62 ac 78 d1 46 2c 99-63 91 85 ed fc f8 cd b5   .b.x.F,.c.......
    00a0 - ea 3f 4d 48 d8 9a 09 ff-1a df dc 97 e6 9d e5 ab   .?MH............
    00b0 - 68 c1 2b ef bd dd 34 33-49 5e c5 cd 03 92 a1 df   h.+...43I^......
    00c0 - 2a 3e b7 8d a0 fd f4 d5-8d 82 49 f9 b2 ea 87 54   *>........I....T

    Start Time: 1732311666
    Timeout   : 7200 (sec)
    Verify return code: 18 (self-signed certificate)
    Extended master secret: no
    Max Early Data: 0
---
read R BLOCK
---
Post-Handshake New Session Ticket arrived:
SSL-Session:
    Protocol  : TLSv1.3
    Cipher    : TLS_AES_256_GCM_SHA384
    Session-ID: AC315650627A465D405707A1447B666B46B50A9E59F27B8C3C3E0FF992A827B7
    Session-ID-ctx:
    Resumption PSK: C8ED07B034721B976319E1ADFBE6E156A198E7904CE52DE5865E6D8C14A4DEEA487CDC42300881524064DB96D4DC99FC
    PSK identity: None
    PSK identity hint: None
    SRP username: None
    TLS session ticket lifetime hint: 7200 (seconds)
    TLS session ticket:
    0000 - 59 cd ed 9f 8c bc be 42-12 5e 03 99 7f a8 a9 b3   Y......B.^......
    0010 - ba 5e 4e ee 3d 43 a5 5f-cd 6a 34 35 d8 c0 b3 62   .^N.=C._.j45...b
    0020 - d4 94 05 0f 01 8e 8f 73-4d e0 57 70 5c 57 fd aa   .......sM.Wp\W..
    0030 - a3 18 9e 8f 38 bc 0e 50-03 f0 00 15 d2 f1 6a 55   ....8..P......jU
    0040 - 58 85 f7 11 5f 66 a2 3a-b4 85 03 14 4c d2 3c 72   X..._f.:....L.<r
    0050 - 23 5c b3 1b aa e1 08 11-50 09 43 fb ce 7a 51 87   #\......P.C..zQ.
    0060 - 9b bb 1d e2 af 2c 06 53-cc 9a 71 fa 4b 6c f5 c2   .....,.S..q.Kl..
    0070 - 7a 1b 4a 37 bd 95 bc 2a-c7 27 f4 7e 7a 26 19 39   z.J7...*.'.~z&.9
    0080 - 93 ec 64 4b 74 95 98 0c-92 d0 5e 86 f9 3a d2 46   ..dKt.....^..:.F
    0090 - 98 f6 d1 cd 12 58 f6 c7-ce 2c a1 76 18 9c 56 58   .....X...,.v..VX
    00a0 - 7c 27 6c f1 16 fc de cf-e6 47 8a e9 70 88 d6 ea   |'l......G..p...
    00b0 - 46 95 7f db eb 1d be 9f-39 4e b7 10 33 74 60 73   F.......9N..3t`s
    00c0 - 47 e4 30 24 af bf a9 5a-f7 94 81 d1 c4 23 6d 02   G.0$...Z.....#m.

    Start Time: 1732311666
    Timeout   : 7200 (sec)
    Verify return code: 18 (self-signed certificate)
    Extended master secret: no
    Max Early Data: 0
---
```

ons seems to use its own ssl library sslnx/sslss.
i think private keys are stored in /u01/app/grid/crsdata/racnoded1/onswallet/cwallet.sso
but the default password is randomly generated so i cant get at the keys.
not sure how oracle reads them either if it doesnt know the password.
