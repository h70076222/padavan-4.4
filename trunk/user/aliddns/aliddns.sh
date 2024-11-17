#!/bin/sh

##编写：hon
##日期：2024-4-25
##版本：2.0
##运行平台：padavan
##运行环境：电信宽带，padaan拨号

sz="$@"

 [ "$1" == "0" ] && {
 killall vpns 
 kill `ps |grep '\-b 39872 -t 127.0.0.1 -p 29872'|grep -v grep|awk '{print $1}'`
 exit 0
 }

:<< eof5
if [ -z "${sz}" ] ; then
echo "请输入dynv6的域名和密码"
logger "请输入dynv6的域名和密码"
exit
fi
eof5

[ ! -d "/tmp/log/" ] &&  mkdir "/tmp/log/" 
[ ! -d "/tmp/vpns/" ] &&  mkdir "/tmp/vpns/" && ln -fs "/tmp/log/" "/tmp/vpns/"

vpns_tmp="/tmp/vpns/log/natmap_vpns_dynv6_txt_tmp"

firewall_ () { 
iptables -C INPUT -p udp --dport 39872 -j ACCEPT &>/dev/null || iptables -A INPUT -p udp --dport 39872 -j ACCEPT
iptables -C INPUT -p udp --dport 29872 -j ACCEPT &>/dev/null || iptables -A INPUT -p udp --dport 29872 -j ACCEPT
 [ -f "/etc/storage/started_script.sh" ] && [ -z "$(cat /etc/storage/post_iptables_script.sh | grep 39872 | grep udp)" ] && {
 echo "iptables -A INPUT -p udp --dport 39872 -j ACCEPT" >> /etc/storage/post_iptables_script.sh 
 }
  [ -f "/etc/storage/started_script.sh" ] && [ -z "$(cat /etc/storage/post_iptables_script.sh | grep 29872 | grep udp)" ] && {
 echo "iptables -A INPUT -p udp --dport 29872 -j ACCEPT" >> /etc/storage/post_iptables_script.sh 
 }
 ## 检查是否开放对应的端口
}


if [[ -f "$vpns_tmp" ]]; then
  vpns_tmp2=$(tail -n 1 "$vpns_tmp")
else
  vpns_tmp2=""
fi

firewall_

if [[ -n "`ps |grep '\-b 39872 -t 127.0.0.1 -p 29872'|grep -v grep|awk '{print $1}'`" ]]; then
natmap_ps=y

fi

if [[ "${sz}" == "${vpns_tmp2}" && -n "$(pidof vpns)" && "$natmap_ps" == "y" ]]; then
  exit
fi

echo "${sz}" > "$vpns_tmp"




if [ -f "/usr/bin/vpns" ] ; then  
	vnts="/usr/bin/vpns"
elif [ -f "/usr/bin/vpns" ] ; then
	vnts="/usr/bin/vpns"
else
	vpns=""
	##上述目录都不存在vnt-cli
fi
## 查找vnt-cli文件
test ! -x "${vpns}" && chmod +x "${vpns}"



if [ "${vpns}" == "" ] ; then
vpns="/usr/bin/vpns"


##判断文件有无执行权限，无赋予运行权限

 [ ! -d "/tmp/log/" ] &&  mkdir "/tmp/log/"

vpns_dirname=$(dirname "${vpns}") # 返回执行文件所在的目录

if [ ! -f /tmp/log/log4rs.yaml ] ; then
cat << 'EOF1' > /tmp/log/log4rs.yaml
refresh_rate: 30 seconds
appenders:
  rolling_file:
    kind: rolling_file
    path: /tmp/vpns.log
    append: true
    encoder:
      pattern: "{d} [{f}:{L}] {h({l})} {M}:{m}{n}"
    policy:
      kind: compound
      trigger:
        kind: size
        limit: 1 mb
      roller:
        kind: fixed_window
        pattern: /tmp/vpns.{}.log
        base: 1
        count: 2

root:
  level: info
  appenders:
    - rolling_file
EOF1

fi

if [ -f "/usr/bin/natmap" ] ; then  
	natmap="/usr/bin/natmap"

elif [ -f "/usr/bin/natmap" ] ; then
	natmap="/usr/bin/natmap"
else
	natmap=""
	##上述目录都不存在vnt-cli
	echo "系统不存在natmap插件"
fi
## 查找vnt-cli文件
test ! -x "${natmap}" && chmod +x "${natmap}"

 
##判断文件有无执行权限，无赋予运行权限

###############################

test -n "`pidof vpns`" && killall vpns

fi
aliddns_interval=$(nvram get aliddns_interval) 
echo $aliddns_interval
aliddns_ttl=$(nvram get aliddns_ttl) 
echo $aliddns_ttl
 
/usr/bin/vpns --port $aliddns_interval --gateway $aliddns_ttl --netmask 255.255.255.0 --web-port 29870 --username admin --password admin &


echo -e " host_name:$1\n host:$2\n token:$3" > /tmp/ip4p_dynv6_vpns_txt.ini

cat <<'EOF2'> /tmp/ip4p_dynv6_vpns_txt.sh
#!/bin/sh

IP=$1
PORT=$2
addr=${IP}:${PORT}
###################



log () {
   logger -t "【Dynv6域名TXT解析】" "$1"
   echo -e "\n\033[36;1m$(date "+%G-%m-%d %H:%M:%S") ：\033[0m\033[35;1m$1 \033[0m"
}

# 这个为解析IP:端口到域名 使用txt方式，nslookup -type=txt 域名 即可解析出地址
# 命令行为 nslookup -type=txt 你的域名 223.5.5.5 | awk '/text/' | awk '{print $4}' | awk -F\" '{print $2}'

#上面域名的前缀（例如 test.abc12345.v6.army）,并需要为此域名先创建一个txt记录 任意值即可 如 "120.66.66.66:52011"
if [ -f /tmp/ip4p_dynv6_vpns_txt.ini ] && [ "`cat /tmp/ip4p_dynv6_vpns_txt.ini | grep 'host_name:' | awk -F 'host_name:' '{print $2}'`" != "" ] ; then
host_name="`cat /tmp/ip4p_dynv6_vpns_txt.ini | grep 'host_name:' | awk -F 'host_name:' '{print $2}'`" 
 [ "$host_name" == "0" ] && exit
host="`cat "/tmp/ip4p_dynv6_vpns_txt.ini" | grep 'host:' | awk -F 'host:' '{print $2}'`" 
token="`cat /tmp/ip4p_dynv6_vpns_txt.ini | grep 'token:'  | awk -F 'token:' '{print $2}'`" 

## 读取/tmp/ip4p_dynv6_vpns_txt.ini的内容

else
aliddns_ak=$(nvram get aliddns_ak) 
echo $aliddns_ak
aliddns_sk=$(nvram get aliddns_sk) 
echo $aliddns_sk
aliddns_name=$(nvram get aliddns_name) 
echo $aliddns_name

host_name=$aliddns_ak
host=$aliddns_sk
token=$aliddns_name


 [  -z "$(echo -e "${host_name}" | tr -d '[:space:]')"  ] && log "host_name:请输入<前缀>" && exit 
 [  -z "$(echo -e "${host}" | tr -d '[:space:]')"  ] && log "host:请输入<域名>" && exit 
 [  -z "$(echo -e "${token}" | tr -d '[:space:]')"  ] && log "token:请输入<令牌>" && exit 


fi

txt="/tmp/ip4p_dynv6_vpns_txt.txt"

test -z "$addr" && exit
test -f "$txt" && oldaddr=$(tail -n 1 $txt | awk -F " " '{print $3}') || oldaddr="::"
[ "$addr" == "$oldaddr" ] && exit 


#################


host_domian="${host_name}.${host}"
log "开始解析${IP}:${PORT} 到 ${host_domian} "

log "开始获取 ${host}  的区域ID..."
zoon_id="$(curl -s -k -X GET \
       -H "Authorization: Bearer $token" \
       -H "Content-Type: application/json" \
       -H "Accept: application/json" \
       https://dynv6.com/api/v2/zones )"
zoonid=$(echo "$zoon_id" | grep -o "{[^}]*\"name\":\"$host\"[^}]*}" | grep -o '"id":[0-9]\+' | grep -o '[0-9]\+')
if [ -z "$zoonid"] ; then
   log "无法获取 ${host} 的区域ID，请检查"
   exit 1
fi
log "${host} 的区域ID：$zoonid"

log "开始获取 ${host_domian} 的记录ID..."
record="$(curl -s -k -X GET \
       -H "Authorization: Bearer $token" \
       -H "Content-Type: application/json" \
       -H "Accept: application/json" \
       https://dynv6.com/api/v2/zones/$zoonid/records)"
#records=$(echo "$record" | grep -o '"type":"TXT","[^}]*' | grep -o "{[^}]*\"name\":\"$host_name\"[^}]*}")
recordid=$(echo "$record" | grep -o '"type":"TXT","[^}]*'| grep "$host_name" | grep -o '"id":[0-9]\+' | grep -o '[0-9]\+')
last_ip=$(echo "$record" | grep -o '"type":"TXT","[^}]*' | grep "$host_name" | grep -oE '"data":"([^"]+)"' | cut -d '"' -f 4)
if [ -z "$recordid"] ; then
   log "无法获取 ${host_domian} 的记录ID，请检查"
   exit 1
fi
log " ${host_domian} 的记录ID：$recordid"
log "${host_domian}  记录的IP：$last_ip"

if [ "$addr" != "$last_ip" ] ; then
  status=$(curl -s -k -X PATCH \
       -H "Authorization: Bearer $token" \
       -H "Content-Type: application/json" \
       -H "Accept: application/json" \
       --data '{"name":"'$host_name'", "data":"'$addr'", "type":"TXT"}' \
       https://dynv6.com/api/v2/zones/$zoonid/records/$recordid)
 statu=$(echo $status | grep -oE '"data":"([^"]+)"' | cut -d '"' -f 4)
  if [ "$addr" = "$statu" ] ; then
     log "更新 ${IP}:${PORT} 到 ${host_domian} 成功！"
     
     echo -e "`date '+%G-%m-%d %H:%M:%S'` $addr" >> $txt
     
     exit 0
  else
     log "更新 ${IP}:${PORT} 到 ${host_domian} 失败，请检查"
     exit 1
  fi
else
  log "当前IP ${addr} 与上次IP ${last_ip} 相同，无需更新！ "
  exit 0
fi

EOF2

 [ ! -x "/tmp/ip4p_dynv6_vpns_txt.sh" ] && chmod +x "/tmp/ip4p_dynv6_vpns_txt.sh"

if [ ! -n "`ps |grep '\-b 39872 -t 127.0.0.1'|grep -v grep |awk '{print $1}'`" ] ; then
echo  "${natmap}  -u -s stun.miwifi.com -b 39872 -t 127.0.0.1 -p 29872 -e /tmp/ip4p_dynv6_vpns_txt.sh >> /tmp/log/ip4p_dynv6_vpns_txt.log &"
 ${natmap} -u -s stun.miwifi.com -b 39872 -t 127.0.0.1 -p 29872 -e /tmp/ip4p_dynv6_vpns_txt.sh >> /tmp/log/ip4p_dynv6_vpns_txt.log &
fi
