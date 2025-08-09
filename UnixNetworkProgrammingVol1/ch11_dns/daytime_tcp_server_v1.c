#include "../util/util.h"
#include "../util/unp.h"

#include <string.h> // memcpy
#include <strings.h> // bzero()
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define CONFIG_RUN_SPECI_SERVER 1 // 通过命令行参数指定运行 ipv6 还是 ipv4 服务器


int Tcp_listen(const char *hostname, const char *service, socklen_t *addrlenp);


/**
 * @brief 使用 tcp_listen 实现
 *        服务端不写主机名,直接填 NULL
 *        e.g. ./daytime_server daytime
 *        端口 13 必须要 sudo 权限,否则异常报错
 *        // NOTE:必须 sudo
 *        ./daytime_server 0.0.0.0 daytime   # 指定运行 ipv4 服务器
 *        ./daytime_server 0::0 daytime     # 指定运行 ipv6 服务器
 * 
*/
int main(int argc, char *argv[])
{
    int listenfd, connfd;
    socklen_t len;
    char buff[MAXLINE];
    time_t ticks;
    struct sockaddr_storage cliaddr;
    char hostname[128], servname[128];
    int n;
    int ret;
    int ipv4_server = 0, ipv6_server = 0;

#if !defined(CONFIG_RUN_SPECI_SERVER)
    if (argc != 2)
        err_quit("usage: daytime_server <service or port>");
#else
    // 通过 inet_pton 函数判断使用ipv4服务器还是 ipv6 服务器
    if (Inet_pton(AF_INET, argv[1], hostname) == 1) {// success
        LOG_I("ipv4 server");
        ipv4_server = 1;
    }
    // inet_pton(AF_INET, "0::0", hostname); /* fail */
    // Inet_pton(AF_INET6, "0.0.0.0", NULL);
    if (Inet_pton(AF_INET6, argv[1], hostname) == 1) {
        LOG_I("ipv6 server");
        ipv6_server = 1;
    }
#endif

    if (argc == 2)
        listenfd = Tcp_listen(NULL, argv[1], NULL);
#if CONFIG_RUN_SPECI_SERVER
    else if (argc == 3)
        listenfd = Tcp_listen(argv[1], argv[2], NULL);
#endif
    else
        err_quit("usage: daytime_server <ipv4addr or ipv6addr> <service or port>");

    LOG_I("server started");

    for (; ;) {
        len = sizeof(cliaddr);
        connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &len);
        LOG_I("connection from %s fd:%d", sock_ntop(&cliaddr, len), connfd);
#if 1
        // getnameinfo 可以用于获取客户端的主机名,不过这通过 DNS 中的 PTR 记录查询实现
        // 而 PTR 记录查询会消耗一定的时间,特别是在 PTR 记录查询失败的情况下.
        // 在一个繁忙的 web 服务器中,不应该出现这里,因为有很多的客户端主机没有主机名导致影响服务器效率
        n = Getnameinfo(&cliaddr, len, hostname, sizeof(hostname), servname, sizeof(servname), 0);
        if (n != 0) {
            LOG_E("getnameinfo failed:%d %s", n, Gai_strerror(n));
        } else {
            LOG_I("hostname:%s service:%s", hostname, servname);
        }
#endif
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        Write(connfd, buff, strlen(buff));
        Close(connfd);
        LOG_I("connection exit fd:%d\n", connfd);
        
    }
}


/**
 * @brief tcp 服务端接口
 * 
 * @param[in]  hostname 主机名
 * @param[in]  service 服务名
 * @param[out] addrlenp
 * @retval
*/
static int tcp_listen(const char *hostname, const char *service, socklen_t *addrlenp)
{
    int listenfd, n;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE; // 套接字用于被动打开,
    hints.ai_family = AF_UNSPEC; // 对于双栈主机返回 ipv4 & ipv6 地址
    hints.ai_socktype = SOCK_STREAM;

    if ( (n = Getaddrinfo(hostname, service, &hints, &res)) != 0) {
        err_quit("tcp_listen Getaddrinfo error for %s %s :%s", hostname, service, gai_strerror(n));
    }
    ressave = res;
    // TODO: 这里是不是有 bug, 如果 getaddrinfo 

    do {
        listenfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0) {/* error, try next one */
            LOG_E("create listen socket failed");
            continue;
        }
        
        Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0) { /* success */
            LOG_I("bind success");
            break;
        }
        Close(listenfd); // bind error, close and try next one
    } while ( (res = res->ai_next) != NULL );

    if (res == NULL)
        err_quit("tcp_listen error for %s, %s", hostname, service);
    
    Listen(listenfd, LISTENQ);

    if (addrlenp)
        *addrlenp = res->ai_addrlen; // return sizeof protocol address length
    
    freeaddrinfo(ressave);

    return (listenfd);
}

/**
 * @brief 为了代码风格一致性
*/
int Tcp_listen(const char *hostname, const char *service, socklen_t *addrlenp)
{
    return tcp_listen(hostname, service, addrlenp);
}
