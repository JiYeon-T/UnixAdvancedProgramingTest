- <Unix Network programing>


- compile
```shell
gcc -g util/util.c util/unp.c  ch4_tcp/tcp_server_v1.c  -w -g -o server -lsctp
gcc -g util/util.c util/unp.c ch4_tcp/tcp_client_v1.c  -w -o client -lsctp
# compile SCTP, need link sctp lib
gcc ch10_sctp/sctp_server_v1.c util/unp.c util/util.c -w -g -o server -lsctp
```

- Linux 蓝牙编程
hciattach  hciconfig  hcitool    

- linux CAN socket 编程
https://www.cnblogs.com/arnoldlu/p/18061649


TOOD:
- learn UNIX network shell command
```shell
traceroute
ifconfig # internet config
iwconfig # wireless config
tcpdump
nsh
rlogin
telnet service

2. multiconnection server, page45
use fork manage different connect, how to realize this function.

3. p50 standard internet services cannot found at my pc
/etc/services
these simple services usually is banned for network security.

TODO:learn 本书中用到 值-结果参数的一部分函数
select 函数中间的3个参数
getsockopt 函数的长度参数
recvmsg 函数中, msghdr 结果痛得 msg_namelen 和 msg_controllen 字段
ifconf 结构痛得 ifc_len 字段
sysctl 函数两个长度参数中的第一个

- 5 种 IO 模型除了异步 IO 其他本书均有涉及
异步 IO 学习 -> aio 以及 libuv 服务端实现;

libuv 这个库需要好好研究一下


- 小米 代码除了底层
上层 LVGL + libuv 这部分都是可以直接在 Linux 上运行的 - sim


# TODO:
##### 4. 整本书看完后再看一遍第七章 套接字选项设置


// TODO: releaize a ipv4 & ipv6 compatiable servr with funciont getaddrinfo()


- 6. webrtc:



##### 5. 一台主机运行多个不同 Ip  的服务器程序的方法 :p83
bind() 捆绑非通配 IP 地址到套接字上的常见例子是在为多个组织提供 web 服务器的主机上.
首先,每个组织都要各自的域名,例如:www.org.com,
其次,每个组织的域名都映射到不同的 ip 地址(DNS), 不过通常在一个子网上.举例来说,主机网络为:198.69.10.0, 第一个组织的 ip 可以是:198.69.10.128, 第二个组织的 ip 可以是:198.69.10.129, 等等其他域名.
然后,把所有这些 Ip 都定义成单个网络接口的别名(比如:使用 ifconfig 命令的 alias 选项来定义),这样,ip 层将收到所有目的地为任何一个别名地址的外来数据报.
最后,为每个组织启动一个 HTTP 服务器的副本,每个副本仅仅捆绑相应组织的 ip 地址. 


##### 6. httpd 服务器
https://zhuanlan.zhihu.com/p/19533379095
https://zhuanlan.zhihu.com/p/10178320684
https://devpress.csdn.net/cloudnative/66d590861328e17ef473491d.html

##### 7. 标注库
```shell
-lrt # 使用异步 io aio_read 时需要链接
-lsctp # sctp


##### 8. FTP 的使用
课后习题:12.1
```
systemctl start vsftpd # 启动服务


ftp localhost


##### 9. telnet 的使用:
```shell
telnet localhost 9999