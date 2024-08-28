#!/bin/sh

##编写：hong

sz="$@"
/usr/bin/vpn --stop
#关闭vnt的防火墙
iptables -D INPUT -i vnt-tun -j ACCEPT 2>/dev/null
iptables -D FORWARD -i vnt-tun -o vnt-tun -j ACCEPT 2>/dev/null
iptables -D FORWARD -i vnt-tun -j ACCEPT 2>/dev/null
iptables -t nat -D POSTROUTING -o vnt-tun -j MASQUERADE 2>/dev/null
killall vpn
killall -9 vpn
sleep 3
#清除vpn的虚拟网卡
ifconfig vnt-tun down && ip tuntap del vnt-tun mode tun

if [ "${vpn}" == "" ] ; then
vpn="/usr/bin/vpn"

test ! -x "${vpn}" && chmod +x "${vpn}"
##判断文件有无执行权限，无赋予运行权限

 [ ! -d "/tmp/log/" ] &&  mkdir "/tmp/log/"

vpn_dirname=$(dirname "${vpn}") # 返回执行文件所在的目录



###############################


test -n "`pidof vpn`" && killall vpn

if [ -f "/etc/storage/post_wan_script.sh" ] ; then
boot="/etc/storage/post_wan_script.sh"

if [ -z "`cat $boot | grep -o '\-k'`" ] ; then
cat <<'EOF10'>> "$boot"
sleep 20 && /usr/bin/vpn.sh &
:<<'________'
VPN异地组网配置区
串码 yhtfhgdf #组网串码 所有同一组网必须同一名 如：abcd
#以下改IP参数，虚似IP最后一位也要对应改，和路由的名要一起改
#如改本地192.168.30.1,路由的名就改30 本机虚拟IP改10.26.0.30
#远端改(另一个路由)192.168.30.0/24,10.26.0.30
#服务器
路由的名 22     #路由的名字，不能和组网同名 如：-d  1 2 3
对网路由IP 192.168.12.0/24,10.26.0.12   # 对端路由IP,对端路由的虚拟IP 例如:192.168.1.0/24,10.26.0.11
本机IP 192.168.33.0/24 #   本路由IP 如:192.168.1.0/24
本机虚拟IP 10.26.0.33 #   本路由的虚拟IP 如:10.26.0.11

________
EOF10

fi

fi
  
 [ -n "`cat $boot | grep -o '\串码'`" ] && port=$(cat $boot | grep '\串码' | awk -F '\串码' '{print $2}'|awk -F '#' '{print $1}' ) 
 [ -n "`cat $boot | grep -o '\-s'`" ] && white=$(cat $boot | grep '\-s' | awk -F '#' '{print $1}' )
 [ -n "`cat $boot | grep -o '\路由的名'`" ] && white_token=$(cat $boot | grep '\路由的名' | awk -F '\路由的名' '{print $2}'|awk -F '#' '{print $1}' )
 [ -n "`cat $boot | grep -o '\对网路由IP'`" ] && gateway=$(cat $boot | grep '\对网路由IP' | awk -F '\对网路由IP' '{print $2}'|awk -F '#' '{print $1}' )
 [ -n "`cat $boot | grep -o '\本机IP'`" ] && netmask=$(cat $boot | grep '\本机IP' | awk -F '\本机IP' '{print $2}'|awk -F '#' '{print $1}' )
 [ -n "`cat $boot | grep -o '\本机虚拟IP'`" ] && finger=$(cat $boot | grep '\本机虚拟IP' | awk -F '\本机虚拟IP' '{print $2}'|awk -F '#' '{print $1}' )

 
echo "./vpn -k $port $white -d $white_token -i $gateway -o $netmask --ip $finger &"

vpn_dirname=$(dirname ${vpn})

cd $vpn_dirname && ./vpn -k $port $white -d $white_token -i $gateway -o $netmask --ip $finger  &

sleep 3
if [ ! -z "`pidof vpn`" ] ; then
logger -t "异地组网" "启动成功"
#放行vnt防火墙
iptables -I INPUT -i vnt-tun -j ACCEPT
iptables -I FORWARD -i vnt-tun -o vnt-tun -j ACCEPT
iptables -I FORWARD -i vnt-tun -j ACCEPT
iptables -t nat -I POSTROUTING -o vnt-tun -j MASQUERADE
#开启arp
ifconfig vnt-tun arp
else
logger -t "异地组网" "启动失败"
fi
