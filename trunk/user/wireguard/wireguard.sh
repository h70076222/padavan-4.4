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
vpn_error="错误：${VPNCLI} 未运行，请运行成功后执行此操作！"
vpn_process=$(pidof vpn)
vpnpath=$(dirname "$VPNCLI")
cmdfile="/tmp/vpn_cmd.log"

hxsdwan_info() {
	if [ ! -z "$vpn_process" ] ; then
		cd $vpnpath
		/usr/bin/vpn --info >$cmdfile 2>&1
	else
		echo "$vpn_error" >$cmdfile 2>&1
	fi
	exit 1
}

hxsdwan_all() {
	if [ ! -z "$vpn_process" ] ; then
		cd $vpnpath
		/usr/bin/vpn --all >$cmdfile 2>&1
	else
		echo "$vpn_error" >$cmdfile 2>&1
	fi
	exit 1
}

hxsdwan_list() {
	if [ ! -z "$vpn_process" ] ; then
		cd $vpnpath
		/usr/bin/vpn --list >$cmdfile 2>&1
	else
		echo "$vpn_error" >$cmdfile 2>&1
	fi
	exit 1
}

hxsdwan_route() {
	if [ ! -z "$vpn_process" ] ; then
		cd $vpnpath
		/usr/bin/vpn --route >$cmdfile 2>&1
	else
		echo "$vpn_error" >$cmdfile 2>&1
	fi
	exit 1
}

hxsdwan_status() {
	if [ ! -z "$vpn_process" ] ; then
		vpncpu="$(top -b -n1 | grep -E "$(pidof vpn)" 2>/dev/null| grep -v grep | awk '{for (i=1;i<=NF;i++) {if ($i ~ /vpn/) break; else cpu=i}} END {print $cpu}')"
		echo -e "\t\t vpn 运行状态\n" >$cmdfile
		[ ! -z "$vpncpu" ] && echo "CPU占用 ${vpncpu}% " >>$cmdfile 2>&1
		vpnram="$(cat /proc/$(pidof vnt-cli | awk '{print $NF}')/status|grep -w VmRSS|awk '{printf "%.2fMB\n", $2/1024}')"
		[ ! -z "$vpnram" ] && echo "内存占用 ${vpnram}" >>$cmdfile 2>&1
		vpntime=$(cat /tmp/vpn_time) 
		if [ -n "$vpntime" ] ; then
			time=$(( `date +%s`-vpntime))
			day=$((time/86400))
			[ "$day" = "0" ] && day=''|| day=" $day天"
			time=`date -u -d @${time} +%H小时%M分%S秒`
		fi
		[ ! -z "$time" ] && echo "已运行 $time" >>$cmdfile 2>&1
		cmdtart=$(cat /tmp/vpn.CMD)
		[ ! -z "$cmdtart" ] && echo "启动参数  $cmdtart" >>$cmdfile 2>&1
		
	else
		echo "$vpn_error" >$cmdfile
	fi
	exit 1
}

case $1 in
start)
	start_vpn &
	;;
stop)
	stop_vpn
	;;
restart)
	stop_vpn
	start_vpn &
	;;
update)
	update_vpn &
	;;
vpninfo)
	vpn_info
	;;
vpnall)
	vpn_all
	;;
vpnlist)
	vpn_list
	;;
vpnroute)
	vpn_route
	;;
vpnstatus)
	vpn_status
	;;
*)
	echo "check"
	#exit 0
	;;
esac
