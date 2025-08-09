#include "../util/util.h"
#include "../util/unp.h"
#include "config.h"

#include <string.h> // memcpy
#include <strings.h> // bzero()
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int Tcp_listen(const char *hostname, const char *service, socklen_t *addrlenp);


/**
 * sudo ./server daytime
*/
int main(int argc, char *argv[])
{
    int listenfd, connfd;
    socklen_t len;
    char buff[MAXLINE];
    struct sockaddr cliaddr;
    socklen_t cliaddrlen;

    if (argc != 2)
        err_quit("error, usage ./server <service>");
    
    listenfd = Tcp_listen(NULL, argv[1], NULL);
    LOG_I("ipv4 tcp server started.");

    for (;;) {
        len = sizeof(cliaddr);
        connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
        LOG_I("connection from %s fd:%d", sock_ntop(&cliaddr, cliaddrlen), connfd);
        bzero(buff, sizeof(buff));
        len = read(connfd, buff, sizeof(buff));
        buff[len] = '\0';
        LOG_I("recv:%s", buff);
        Write(connfd, buff, strlen(buff) + 1);
        Close(connfd);
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
    hints.ai_family = AF_INET; /* IPV6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP */ 

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
