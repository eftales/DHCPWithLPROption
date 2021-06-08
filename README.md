# DHCP Client with LPR Option

一般而言，DHCP服务器只会返回DHCP客户端请求的字段，其他字段如果客户端不请求的话就不会返回这个字段。

本demo展示了如何在 Option 中增加请求 LPR 的字段。

## config DHCP Server
```
sudo apt install isc-dhcp-server
sudo gedit /etc/default/isc-dhcp-server 
```

修改 /etc/default/isc-dhcp-server 中的 `INTERFACE="以太网口的名称"` .

```
sudo gedit /etc/dhcp/dhcpd.conf
```

在 /etc/dhcp/dhcpd.conf 文件中添加你对DHCP服务器的配置，典型配置如下

```
subnet 192.168.0.1 netmask 255.255.255.0 {

range 192.168.0.10 192.168.0.40;

option domain-name-servers 8.8.8.8, 4.4.4.4;

option routers 192.168.0.1;

option broadcast-address 192.168.0.255;

default-lease-time 600;

max-lease-time 7200;

host gateway {

hardware ethernet 00:00:27:7e:3a:73;

fixed-address 192.168.0.40;

}
}
```

```
sudo service isc-dhcp-server restart
vim /var/log/syslog # 查看运行日志，如果报错了也会在这里显示
```
