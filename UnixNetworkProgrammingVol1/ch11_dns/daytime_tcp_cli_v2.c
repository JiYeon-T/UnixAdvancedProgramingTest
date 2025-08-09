
#include "../util/util.h"
#include "../util/unp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h> // memcpy
#include <strings.h> // bzero()


int Tcp_connect(const char *host, const char *serv);


/**
 * @brief 使用 tcp_connect 重写该客户端程序
 *        e.g. ./daytime_client baidu.com domain
*/
int main(int argc, char *argv[])
{
    int sockfd, n;
    char recvline[MAXLINE + 1];
    socklen_t len;
    struct sockaddr_storage ss;
    
    if (argc != 3) {
        // e.g. daytimetcpcli localhost daytime
        err_quit("usage:daytimetcpcli <hostname> <service>");
    }

    sockfd = Tcp_connect(argv[1], argv[2]);
    if (sockfd < 0) {
        LOG_E("connect failed");
        return -1;
    }

    len = sizeof(ss);
    Getpeername(sockfd, &ss, &len);
    // printf("connect to %s\n", sock_ntop_host((struct sockaddr *)&ss, len))
    printf("connect to %s fd:%d\n", sock_ntop((const struct sockaddr*)&ss, len), sockfd);

    while (n = Read(sockfd, recvline, sizeof(recvline))) {
        recvline[n] = '\0';
        printf("read len:%d\n", n);
        Fputs(recvline, stdout);
    }
    printf("client %d exit\n", sockfd);
    exit(0);    
}


/**
 * @brief  tcp 客户端接口, 通过域名 和 服务名建立 tcp 连接
 * 
 * @param[in]  host
 * @param[in]  serv
 * 
*/
static int tcp_connect(const char *host, const char *serv)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // 返回两个套接字地址结构:ipv4 & ipv6
    hints.ai_socktype = SOCK_STREAM;

    if ( (n = Getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("tcp connect error for %s, %s : %s", host, serv, gai_strerror(n));
    ressave = res;
    
    do {
        sockfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;
        if ( connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
            LOG_I("connect success %s fd:%d", sock_ntop(res->ai_addr, res->ai_addrlen),
                    sockfd);
            break;
        }
        Close(sockfd); // ignore this one
    } while ( (res = res->ai_next) != NULL );

    FreeAddrinfo(ressave);

    return (sockfd);
}

/**
 * @brief 为了代码风格一致性, 再封装一个 Tcp_connect
*/
int Tcp_connect(const char *host, const char *serv)
{
    return tcp_connect(host, serv);
}
