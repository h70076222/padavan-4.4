#!/bin/sh


/usr/bin/vpn --stop
#关闭vnt的防火墙
iptables -D INPUT -i vnt-tun -j ACCEPT 2>/dev/null
iptables -D FORWARD -i vnt-tun -o vnt-tun -j ACCEPT 2>/dev/null
iptables -D FORWARD -i vnt-tun -j ACCEPT 2>/dev/null
iptables -t nat -D POSTROUTING -o vnt-tun -j MASQUERADE 2>/dev/null
killall vpn
killall -9 vpn
sleep 3
#清除vnt的虚拟网卡
ifconfig vnt-tun down && ip tuntap del vnt-tun mode tun
#启动命令 更多命令去官方查看
wireguard_key=$(nvram get wireguard_key) 
echo $wireguard_key
wireguard_naen=$(nvram get wireguard_naen) 
echo $wireguard_naen
wireguard_inip=$(nvram get wireguard_inip) 
echo $wireguard_inip
wireguard_outip=$(nvram get wireguard_outip) 
echo $wireguard_outip
wireguard_ttre=$(nvram get wireguard_ttre) 
echo $wireguard_ttre
lan_ipaddr=$(nvram get lan_ipaddr) 
echo $lan_ipaddr

/usr/bin/vpn -k $wireguard_key $wireguard_ttre -d $wireguard_naen -i $wireguard_inip -o $lan_ipaddr/24 --ip $wireguard_outip &

sleep 3
if [ ! -z "`pidof vpn`" ] ; then
logger -t "组网" "启动成功"
#放行vpn防火墙
iptables -I INPUT -i vnt-tun -j ACCEPT
iptables -I FORWARD -i vnt-tun -o vnt-tun -j ACCEPT
iptables -I FORWARD -i vnt-tun -j ACCEPT
iptables -t nat -I POSTROUTING -o vnt-tun -j MASQUERADE
#开启arp
ifconfig vnt-tun arp
else
logger -t "组网" "启动失败"
fi
