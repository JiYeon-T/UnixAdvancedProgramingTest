#include <netinet/in.h>

#include "../util/util.h"
#include "../util/unp.h"

#include "config.h"
        
// 双栈主机,同时支持 Ipv4 与 ipv6

// NOTE: 大多数双栈主机在处理套接字监听时满足一下规则:
// 1. ipv4 套接字只能接受来自 ipv4 客户的连接. 因为,一般来说,一个 ipv6 地址无法转换成一个 ipv4 地址
// 2. 如果服务器有一个绑定了通配地址的 ipv6 监听套接字,而且该套接字未设置 IPV6_V6ONLY 套接字选项
// 那么该套接字既能接受来自 ipv4 客户的外来连接,又可以接受来自 ipv6 客户的外来连接
// 对于 ipv4 客户的连接而言,其服务端的本地地址(accept 返回(tcp)/recvfrom 返回(udp))
// 将是与某个本地 ipv4 地址对应的 "ipv4映射的ipv6套接字"
// 3.如果服务器有一个 ipv6 监听套接字,而且绑定在其上的是除 "ipv4映射的 ipv6 地址外的某个地址"或者
// 绑定的是 IN6ADDR_ANY_INIT 通配地址,但是设置了 IPV6_V6ONLY 套接字选项,那么该套接字只能接受来自
// ipv6 客户端的连接.

int main(int argc, char *argv[])
{
    int sockfd;
    char buff[MAXLINE];
    int n;

    if (argc != 3)
        err_quit("usage: client <hostname> <service>");

#if IPV4_TCP_SERVER /* ipv4 客户端连接 ipv4 tcp server */
    struct addrinfo hints, *res, *ressave;

    //NOTE: gethostbyname() 仅支持 ipv4
    bzero(&hints, sizeof(hints));
    // 对于 IPV6 服务器,既有 A 记录又有 AAAA 记录,这里仅查找 A 记录
    // 拿到一个 ipv4 地址以及端口号
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags
    Getaddrinfo(argv[1], argv[2], &hints, &res);
    ressave = res;
    LOG_I("get ipv4 server address success: %s", sock_ntop(res->ai_addr, res->ai_addrlen));

    do {
        LOG_I("%d %d %d", res->ai_family, res->ai_socktype, res->ai_protocol);
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) {
            LOG_E("create socket failed");
            continue;
        }
        if (connect(sockfd, (struct sockaddr*)res->ai_addr, res->ai_addrlen) == 0) {
            // cliaddrlen = sizeof(cliaddr);
            // NOTE: 多此一举,画蛇添足
            // Getsockname(sockfd, &cliaddr, &cliaddrlen); // 获取对端地址
            LOG_I("connect success fd:%d ", sockfd);
            break;
        } else {
            LOG_E("connect failed error:%s", strerror(errno));
        }
        Close(sockfd);
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL) {
        LOG_E("failed");
        return -1;
    }
#elif (IPV4_UDP_SERVER == 1) /* ipv4 客户端连接 udp 服务器 */

#endif /* IPV4_TCP_SERVER */


    strncpy(buff, "Hello", sizeof(buff));
    Write(sockfd, buff, strlen(buff) );


    bzero(buff, sizeof(buff));
    n = Read(sockfd, buff, sizeof(buff));
    buff[n] = '\0';
    LOG_I("recv from %d data:%s", sockfd, buff);
    Close(sockfd);
    FreeAddrinfo(ressave);
    LOG_I("client exit");

    return 0;
}
