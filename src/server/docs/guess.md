# guesses about format

## setup
```sh
wsl hostname -I
ssh -J elliothb@172.24.173.47 root@172.16.1.100 -Y
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

### 111 - sunrpc
not sure what uses this

### 1521 - ora (listed as ncube-lm in netstat)
orcl external listener

### 1525 - orasrv (listed as prospero-np in netstat)
orcl connection manager. communication between racnodedc1-cman and racnoded(1/2)
for managing incoming connections.

## packet1
sent from 169.254.16.216:9294 to 169.254.1.169:63675
packet1 is a DCE/RPC packet, seems oracle uses it for some stuff

## packet2
from 169.254.16.216:43863 to 169.254.1.169:56544
looks like a query, i think the placeholder values are first and then the query text after that.
lots of padding for some reason

## packet3
Src: 169.254.1.169, Dst: 169.254.16.216
Src Port: 29292, Dst Port: 26932
    Length: 3480
    UDP payload (3472 bytes)

sent just before packet2 and contains mostly the same content
maybe a response to a query includes the query body?