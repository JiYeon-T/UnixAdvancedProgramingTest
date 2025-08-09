
#include "../util/util.h"
#include "../util/unp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h> // memcpy
#include <strings.h> // bzero()


int Udp_client(const char *host, const char *serv, struct sockaddr **saptr, socklen_t *lenp);
int Udp_connect(const char *host, const char *serv);


/**
 * 
 * sudo ./daytime_client localhost daytime
 * 
*/
int main(int argc, char *argv[])
{
    int sockfd, n;
    char recvline[MAXLINE + 1];
    socklen_t salen;
    struct sockaddr *sa;
    struct sockaddr server_addr;

    if (argc != 3)
        err_quit("usage: daytimeclient <hostname/ipaddr> <service/port>");
    
    sockfd = Udp_client(argv[1], argv[2], (void **)&sa, &salen);
    LOG_I("sending to %s sockfd:%d\n", sock_ntop(sa, salen), sockfd);

    Sendto(sockfd, "", 1, 0, sa, salen);

    n = Recvfrom(sockfd, recvline, sizeof(recvline), 0, NULL, NULL);
    recvline[n] = '\0';
    LOG_I("recv:%s", recvline);

    exit(0);
}


/**
 * @brief 创建一个未连接的 udp 套接字
 * 
 * @param[in]  host 主机名
 * @param[in]  serv 服务名
 * @param[out] saptr 客户端地址
 * @param[out] lenp 地址长度
*/
int Udp_client(const char *host, const char *serv, struct sockaddr **saptr, socklen_t *lenp)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // UDP
    if ( (n = Getaddrinfo(host, serv, &hints, &res)) != 0) {
        err_quit("udp_client error for %s,%s %s", host, serv, Gai_strerror(n));
    }
    ressave = res;

    do {
        sockfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd >= 0)
            break;
    } while ( (res = res->ai_next) != NULL );

    if (res == NULL)
        err_sys("udp_client iterate error for %s %s", host, serv);
    
    *saptr = Malloc(res->ai_addrlen);
    memcpy(*saptr, res->ai_addr, res->ai_addrlen);
    *lenp = res->ai_addrlen;

    freeaddrinfo(ressave);

    return (sockfd);
}


/**
 * @brief 创建已连接 udp 套接字,调用者可改用调用 write 代替 sendto
 * 
 * @param[in]  host 主机名
 * @param[in]  serv 服务名
*/
int Udp_connect(const char *host, const char *serv)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // UDP
    if ( (n = Getaddrinfo(host, serv, &hints, &res)) != 0) {
        err_quit("udp_connect error for %s,%s:%s", host, serv, Gai_strerror(n));
    }
    ressave = res;

    do {
        sockfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) // ignore
            continue;
        // TODO: 在 udp 套接字上调用 connect 的作用
    
        // 在 udp 套接字上调用 connect 不会发送任何数据到服务端
        // 但是可以直接调用 write 了??
        if (connect(sockfd, res->ai_addrlen, res->ai_addrlen) == 0)
            break; // success
        Close(sockfd); // failed
    } while ( (res = res->ai_next) != NULL);

    freeaddrinfo(ressave);

    return (sockfd);
}