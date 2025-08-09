#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"

#include <netinet/in.h>

#define GET_IP_BY_DNS_NAME          0 // 通过域名获取主机 ip
#define GET_SERVNAME_BY_IP          0 // 通过 ip 获取主机名称
#define GET_PORT_BY_SERVICE_NAME    0 // 通过服务名获取端口号
#define GET_SERVICE_NAME_BY_PORT    0 // 通过端口号获取服务名
#define GETADDRINFO_TEST            0
#define GET_HOST_TEST               1 // 主机 api 测试
#define GET_PROTOCOL_TEST           1 // 协议
#define GET_SERV_TEST               1 // 服务
#define GET_NETWORK_TEST            1 // 网络


// #define	_PATH_HEQUIV		"/etc/hosts.equiv"
// 主机:
// #define	_PATH_HOSTS		"/etc/hosts"
// 网络:
// #define	_PATH_NETWORKS		"/etc/networks"
// #define	_PATH_NSSWITCH_CONF	"/etc/nsswitch.conf"
// 协议:
// #define	_PATH_PROTOCOLS		"/etc/protocols"
// 服务:
// #define	_PATH_SERVICES		"/etc/services"

// NOTE:每类信息都有自己定义的结构: hostent, netent, protoent, servent
// 只有主机和网络信息可以通过 DNS 获取,协议和服务信息都是从相应的文件中获取到的
// 不同的实现有不同的方法供管理员指定是使用 DNS 还是使用文件来查找主机和网络信息
// 2. 每类信息都提供了两种不同的查找方法: 1. api 查找; 2.键值查找


/**
 * /etc/hosts include local device's staitc host name and ip reflection.
 * /etc/resolv.conf   local name server's ip address
 * 
 * e.g.
 * ./dns_test localhost
 * ./dns_test qz
 * ./dns_test www.baidu.com
 * 
*/
int main(int argc, char *argv[])
{
#if GET_IP_BY_DNS_NAME
    char *ptr, **pptr;
    char str[INET_ADDRSTRLEN];
    struct hostent *hptr;
    struct servent *sptr;

    if (argc < 2) {
        err_quit("usage ./a.out <hostname>");
        return -1;
    }

    while (--argc > 0) {
        ptr = *++argv;
        if ( (hptr = gethostbyname(ptr)) == NULL) {
            // gethostbyname() does not use errno and strerror but use
            // h_errno & hstrerror()
            err_sys("gethostbyname error for host %s:%s", ptr, hstrerror(h_errno));
            continue;
        }
        print_host_info(hptr);
#if 0 // 使用函数 print_host_info 替代
        printf("official hostname:%s\n", hptr->h_name);

        for (pptr = hptr->h_aliases; *pptr != NULL; pptr++) // alias list
            printf("\talias:%s\n", *pptr);
        
        switch (hptr->h_addrtype) {
            case AF_INET: {
                pptr = hptr->h_addr_list;
                for (; *pptr != NULL; pptr++) { // alias address list
                    printf("\taddress:%s\n", Inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
                }
                break;
            default:
                err_ret("unknown adress type");
                break;
            }
        }
#endif
    }
#endif /* GET_IP_BY_DNS_NAME */

#if GET_SERVNAME_BY_IP
        // char serv_ipv4_addr_p[INET_ADDRSTRLEN] = "127.0.0.1";
        // char serv_ipv4_addr_p[INET_ADDRSTRLEN] = "127.0.1.1";
        // TODO:通过 baidu 的 ip 查找百度的主机名的时候查不到,应该是服务器拒绝了
        // 百度的 DNS 服务器中没有 arpa 反向解析功能? 没有 PTR 资源记录??? 
        char serv_ipv4_addr_p[INET_ADDRSTRLEN] = "153.3.238.28"; 
        struct in_addr serv_ipv4_addr_n;
        if (Inet_pton(AF_INET, serv_ipv4_addr_p, &serv_ipv4_addr_n) != 1) {
            err_quit("ipv4 address exchange Failed");
        }
        memset(serv_ipv4_addr_p, 0, sizeof(serv_ipv4_addr_p));
        Inet_ntop(AF_INET, &serv_ipv4_addr_n, serv_ipv4_addr_p, sizeof(serv_ipv4_addr_p));
        // 通过一个二进制的 ip 地址,获取主机名; 在 in_addr.arpa 域中向一个名字服务器查询 PTR 记录
        // 参数:addr  是 in_addr 类型, len 是结构体大小
        hptr = gethostbyaddr(&serv_ipv4_addr_n, sizeof(struct in_addr), AF_INET);
        if (hptr != NULL) {
            printf("host name:%s\n", hptr->h_name);
        } else {
            err_sys("gethostbyadddr failed for:%s  %s", serv_ipv4_addr_p, hstrerror(h_errno));
        }
#endif /* GET_SERVNAME_BY_IP */

#if GET_PORT_BY_SERVICE_NAME
    // cat /etc/services
    // 端口号规范列表: www.iana.org/assignments/port-numbers
    // 服务名字到端口号的映射在 /etc/services 文件中定义
    sptr =  getservbyname("domain", "udp"); // DNS using UDP
    print_service_info(sptr);
    sptr =  getservbyname("ftp", "tcp"); // FTP using TCP
    print_service_info(sptr);
    sptr =  getservbyname("ftp", NULL); // FTP using TCP
    print_service_info(sptr);
    sptr =  getservbyname("ftp", "udp"); // this call will fail
    print_service_info(sptr);
#endif /* GET_PORT_BY_SERVICE_NAME */

#if GET_SERVICE_NAME_BY_PORT
    // 根据指定端口号和可选协议查找指定服务
    // NOTE:端口号必须是网咯字节序
    sptr = getservbyport(htons(53), "udp"); // DNS using UDP
    print_service_info(sptr);
    sptr = getservbyport(htons(21), "tcp"); // FTP using TCP
    print_service_info(sptr);
    sptr = getservbyport(htons(21), "tcp"); // FTP using TCP
    print_service_info(sptr);
    sptr = getservbyport(htons(21), "udp"); // this call will fail, 应该是新增了协议: fsp 21/udp fspd
    print_service_info(sptr);
    // NOTE: 有些端口 TCP 上用于一种服务, udp 上确用于完全不同的另一种服务, e.g. 514 端口
    // 188:shell		514/tcp		cmd		# no passwords used
    // 189:syslog		514/udp

#endif /* GET_SERVICE_NAME_BY_PORT */

#if GETADDRINFO_TEST
    struct addrinfo hints, *res;
    bzero(&hints, sizeof(hints));
    // ai_flags: 
    //AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST, AI_NUMERICSERV, AI_V4MAPPED, AI_ALL, AI_ADDRCONFIG
    // hints.ai_flags = AI_CANONNAME; // 查找主机的规范名字, 这里为什么报错?
    hints.ai_family = AF_INET;
    Getaddrinfo(argv[1], "domain", &hints, &res);
#endif /* GETADDRINFO_TEST */

#if GET_HOST_TEST
    // 键值对查找函数:gethostbyaddr(), gethostbyname()
    struct hostent *ph;
    ph = gethostent();
    print_host_info(ph);
    ph = gethostent();
    print_host_info(ph);
    endhostent();
#endif /* GET_HOST_TEST */

#if GET_SERV_TEST
    // 键值对查找函数:getservbyname(), getservbyport()
    struct servent *ps;
    ps = getservent();
    print_service_info(ps); // 返回文件中下一个服务
    ps = getservent();
    print_service_info(ps);
    endservent();
#endif /* GET_SERV_TEST */

#if GET_NETWORK_TEST
    // 键值对查找函数:getnetbyaddr(), getnebyname()
    struct network *pn;
    pn = getnetent();
    print_network_info(pn);
    pn = getnetent();
    print_network_info(pn);
    endnetent();
#endif

#if GET_PROTOCOL_TEST
    // 键值对查找函数:getprotobyname(), getprotobynumber()
    struct protoent *pp;
    pp =getprotoent();
    print_protocol_info(pp);
    pp =getprotoent();
    print_protocol_info(pp);
    endprotoent();
#endif /* GET_PROTOCOL_TEST */

    return 0;
}
