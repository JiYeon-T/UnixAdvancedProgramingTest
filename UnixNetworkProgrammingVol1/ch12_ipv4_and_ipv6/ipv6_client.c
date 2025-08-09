#include "../util/util.h"
#include "../util/unp.h"

#include <string.h> // memcpy
#include <strings.h> // bzero()
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "config.h"

#define CONNECT_IPV4_SERVER 1


/**
 * @brief
 *      ./client localhost daytime
*/
int main(int argc, char *argv[])
{
    int sockfd;
    char buff[MAXLINE];
    int n;

    if (argc != 3)
        err_quit("usage: client <hostname> <service>");

#if IPV6_TCP_CLIENT /* ipv6 客户端连接 ipv6 tcp server */
    struct addrinfo hints, *res, *ressave;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET6; // 对于 IPV6 服务器,既有 A 记录又有 AAAA 记录,这里仅查找 A 记录
    hints.ai_socktype = SOCK_STREAM;
#if CONNECT_IPV4_SERVER 
    // ipv6 客户端连接 ipv4 服务器
    // 得到一个 ipv4 映射的 ipv6 地址后可以正常连接, 直接用 ipv6 地址无法连接
    hints.ai_flags = AI_V4MAPPED; 
#endif

    Getaddrinfo(argv[1], argv[2], &hints, &res);
    ressave = res;
    LOG_I("get ipv6 server address success:%s", sock_ntop(res->ai_addr, res->ai_addrlen));
    // TODO:
    // 为什么这个宏返回的一直是 0 ?????????????
    LOG_I("V4_MAPPED:%d", IN6_IS_ADDR_V4MAPPED(res->ai_addr));

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
#elif IPV6_UDP_CLIENT == 1 /* IPV6_TCP_CLIENT */

#endif /* IPV6_UDP_CLIENT */

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