#include "../util/util.h"
#include "../util/unp.h"



// gateway:192.168.1.1
// inet addr:192.168.1.7  Bcast:192.168.1.255  Mask:255.255.255.0

#ifdef IPV4_TEST
/**
 * @brief IPV4 get time server
 *
*/
int main(int argc, char *argv[])
{
    int listenfd, connfd; // 监听套接字以及已连接套接字
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char buf[MAXLINE];
    time_t ticks;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 通配地址(wildcard) INADDR_ANY:can accept many connection on different network at the same host
    servaddr.sin_port = htons(13); // 13 号端口为系统使用, 使用需要超级用户权限, 否则 bind 失败 

    Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    Listen(listenfd, LISTENQ);

    LOG_D("server start");
    for (;;) {
        // accept() will return after TCP's three-way handshake
        // 这里不关心客户端的地址信息因此, cliaddr 参数为 NULL
#if 0
        connfd = Accept(listenfd, (struct sockaddr*)NULL, NULL);
#else // 打印客户端地址信息
        len = sizeof(cliaddr);
        connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &len);
        Inet_ntop(AF_INET, &cliaddr.sin_addr, buf, sizeof(buf));
        LOG_I("connect from %s port %d fd:%d", buf, ntohs(cliaddr.sin_port), connfd);
#endif
        ticks = time(NULL);
        snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
        Write(connfd, buf, strlen(buf));
        Close(connfd);
        LOG_D("client exit connfd:%d", connfd);
    }

    return 0;
}
#elif defined(IPV6_TEST)
/**
 * @brief get time server
 *        ipv6
 *
*/
int main(int argc, char *argv[])
{
    int listenfd, connfd;
    struct sockaddr_in6 servaddr, cliaddr;
    char buf[MAXLINE];
    time_t ticks;
    socklen_t len;

    listenfd = Socket(AF_INET6, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any; // in6addr_any:通配地址, 初始化为 IN6ADDR_ANY_INIT
    servaddr.sin6_port = htons(13);

    Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    Listen(listenfd, LISTENQ);

    LOG_D("daytime server(ipv6) start");
    for (;;) {
        // accept() will return after TCP's three-way handshake
        len = sizeof(cliaddr);
        connfd = Accept(listenfd, (struct sockaddr*)&cliaddr, &len);
        Inet_ntop(AF_INET6, &cliaddr.sin6_addr, buf, sizeof(buf));
        LOG_I("connect from %s port %d", buf, ntohs(cliaddr.sin6_port));
        ticks = time(NULL);
        snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
        Write(connfd, buf, strlen(buf));
        Close(connfd);
        LOG_D("connfd:%d", connfd);
    }

    return 0;
}
#else

#endif
