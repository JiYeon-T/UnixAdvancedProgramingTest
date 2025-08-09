#include "../util/util.h"
#include "../util/unp.h"

#include <string.h> // memcpy
#include <strings.h> // bzero()
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int Udp_server(const char *host, const char *serv, socklen_t *addrlenp);

// NOTE:errno 和 h_errno 存在 "重入问题", 一个进程中的不同线程之间, errno 是共享的
//  因此在信号回调中最好这样操作
void sig_alm_handler(int signo)
{
    int errno_save = errno;

    // if (xxx != nbytes)
    //     fprintf(stderr, "errno:%d", errno);
    errno = errno_save;

    return ;
}

/**
 * 
 * sudo ./datime_server daytime
 * sudo ./daytime_servr localhost daytime
*/
int main(int argc, char *argv[])
{
    int sockfd;
    ssize_t n;
    char buff[MAXLINE];
    time_t ticks;
    socklen_t len;
    struct sockaddr_storage cliaddr;

    if (argc == 2)
        sockfd = Udp_server(NULL, argv[1], NULL);
    else if (argc == 3)
        sockfd = Udp_server(argv[1], argv[2], NULL);
    else
        err_quit("usage daytime_server [<host>] <service or port>");
    
    LOG_I("daytime udp server start\n");
    for (; ;) {
        len = sizeof(cliaddr);
        n = Recvfrom(sockfd, buff, sizeof(buff), 0, (struct sockaddr *)&cliaddr, &len);
        LOG_I("datagram from %s fd:%d\n", sock_ntop((const struct sockaddr*)&cliaddr, len), sockfd);

        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        Sendto(sockfd, buff, sizeof(buff), 0, 
                (const struct sockaddr *)&cliaddr, len);
    }
}

/**
 * @brief udp 服务端创建已经绑定成功的套接字
 * 
 * @param[in]  host 主机名
 * @param[in]  serv 服务名
 * @param[out] addrlenp 返回地址大小
*/
int Udp_server(const char *host, const char *serv, socklen_t *addrlenp)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // udp

    if ( (n = Getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("udp server failed for %s, %s:%s", host, serv, Gai_strerror(n));
    ressave = res;

    do {
        // TODO:
        // 既然 UDP 套接字没有 TCP 的 TIME_WAIT 状态的类似物,启动服务器时也就没有设置 SO_REUSEADDR 选项的必要了
        sockfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) // error, try next one
            continue;

        if ( bind(sockfd, res->ai_addr, res->ai_addrlen) == 0 )
            break; // success
        
        Close(sockfd); // bind error - close and tye next one
    } while ( (res = res->ai_next) != NULL );

    if (res == NULL)
        err_sys("udp_server error for %s, %s", host, serv);
    
    if (addrlenp)
        *addrlenp = res->ai_addrlen; // return size of protocol address 
    
    freeaddrinfo(ressave);

    return (sockfd);
}
