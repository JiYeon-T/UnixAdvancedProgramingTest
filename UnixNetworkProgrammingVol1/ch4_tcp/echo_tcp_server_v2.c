#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"

// #include <sys
#include <poll.h>
#include <sys/stropts.h>


#define ECHO_SERV_PORT                      9999 // echo 服务测试端口
#define SERVER_SINGLE_PROCESS_MULTI_CONN    1 // 使用 select 实现监听多客户端的输入, 不使用多进程
#define SERVER_USE_SELECT                   0 // 有新的客户端连接时,不创建子进程,将 connfd 和 listenfd 都放到 select 集合中,在一个线程中监听
#define SERVER_USE_PSELECT                  0 // pselect
#define SERVER_USE_POLL                     1 // poll

#if SERVER_USE_SELECT
/**
 * @brief 使用 select 实现监听多客户端的输入, 不使用多进程
 *        最大的问题就是可监听的描述符数量限制, FD_SETSIZE,改为 poll() 可解决上述问题
*/
int main(int argc, char *argv[])
{
    int i, maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE]; // 不同操作系统支持的 fd_set 中描述符最大值
    ssize_t n;
    fd_set rset, allset;
    char buff[MAXLINE];
    socklen_t client_addr_len;
    struct sockaddr_in clientaddr, servaddr;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

    maxfd = listenfd;
    maxi = -1; // index into client[] array
    for (i = 0; i < FD_SETSIZE; ++i) // -1 indicates available entry
        client[i] = -1;
    
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset); // 初始情况下仅有 listenfd 需要监听
    LOG_I("listenfd:%d", listenfd);

    /* Maximum number of file descriptors in `fd_set'.  */
    // TODO: 这怎么做到的??? 可以容纳 1024 个描述符,但是结构体的大小是 128
    // LOG_D("FD_SETSIZE:%d s:%d", FD_SETSIZE, sizeof(fd_set)); // 1024, 128

    LOG_I("server started");
    for (;;) {
        // NOTE: fs_set 是值结果参数
        // select 每次重新调用前都需要重新初始化 set 集合門, 因为会清除
        rset = allset;
        nready = Select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset)) { // 当 listenfd 可读的时候就是有新的客户端连接了,new client connection
            client_addr_len = sizeof(clientaddr);
            connfd = Accept(listenfd, (struct sockaddr*)&clientaddr, &client_addr_len);
            for (i = 0; i < FD_SETSIZE; ++i) {
                if (client[i] < 0) {
                    client[i] = connfd; // save client descriptor
                    break;
                }
            }
            if (i == FD_SETSIZE)
                err_quit("too many clients");
            LOG_I("new client connected:%d", connfd);

            FD_SET(connfd, &allset); // add new descriptor to select
            if (connfd > maxfd)
                maxfd = connfd; // for select
            if (i > maxi)
                maxi = i; // max index in client[] array
            if (--nready <= 0) // no more readable descriptors
                continue;
        }

        for (i = 0; i <= maxi; ++i) { // check all clients for data, client 断开后不减小 maxi, 这里会引入多余的遍历
            if ((sockfd = client[i]) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) { // client[i] is readable
                if ((n = Read(sockfd, buff, sizeof(buff))) == 0) { // connection closed by client
                    LOG_I("client %d exit", sockfd);
                    Close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1; // 客户端退出后,client 设置为 -1
                } else {
                    LOG_D("server recv from[%d] len:%d %s", sockfd, n, buff);
                    Writen(sockfd, buff, n);
                }
                if (--nready <= 0)  // no more readable descriptors
                    break;
            }
        }
    }

}

#elif SERVER_USE_PSELECT

// TODO: 使用 pselect 的原因以及方法
// p143
// ch20.7


int main(int argc, char *argv[])
{
    int i, maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE]; // 不同操作系统支持的 fd_set 中描述符最大值
    ssize_t n;
    fd_set rset, allset;
    char buff[MAXLINE];
    socklen_t client_addr_len;
    struct sockaddr_in clientaddr, servaddr;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

    maxfd = listenfd;
    maxi = -1; // index into client[] array
    for (i = 0; i < FD_SETSIZE; ++i) // -1 indicates available entry
        client[i] = -1;
    
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset); // 初始情况下仅有 listenfd 需要监听
    LOG_I("listenfd:%d", listenfd);

    /* Maximum number of file descriptors in `fd_set'.  */
    // TODO: 这怎么做到的??? 可以容纳 1024 个描述符,但是结构体的大小是 128
    // LOG_D("FD_SETSIZE:%d s:%d", FD_SETSIZE, sizeof(fd_set)); // 1024, 128

    LOG_I("server started");
    for (;;) {
        // NOTE: fs_set 是值结果参数
        // select 每次重新调用前都需要重新初始化 set 集合門, 因为会清除
        rset = allset;
        nready = Select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset)) { // 当 listenfd 可读的时候就是有新的客户端连接了,new client connection
            client_addr_len = sizeof(clientaddr);
            connfd = Accept(listenfd, (struct sockaddr*)&clientaddr, &client_addr_len);
            for (i = 0; i < FD_SETSIZE; ++i) {
                if (client[i] < 0) {
                    client[i] = connfd; // save client descriptor
                    break;
                }
            }
            if (i == FD_SETSIZE)
                err_quit("too many clients");
            LOG_I("new client connected:%d", connfd);

            FD_SET(connfd, &allset); // add new descriptor to select
            if (connfd > maxfd)
                maxfd = connfd; // for select
            if (i > maxi)
                maxi = i; // max index in client[] array
            if (--nready <= 0) // no more readable descriptors
                continue;
        }

        for (i = 0; i <= maxi; ++i) { // check all clients for data, client 断开后不减小 maxi, 这里会引入多余的遍历
            if ((sockfd = client[i]) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) { // client[i] is readable
                if ((n = Read(sockfd, buff, sizeof(buff))) == 0) { // connection closed by client
                    LOG_I("client %d exit", sockfd);
                    Close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1; // 客户端退出后,client 设置为 -1
                } else {
                    LOG_D("server recv from[%d] len:%d %s", sockfd, n, buff);
                    Writen(sockfd, buff, n);
                }
                if (--nready <= 0)  // no more readable descriptors
                    break;
            }
        }
    }

}
#elif SERVER_USE_POLL
int main(int argc, char *argv[])
{
    long open_max; // 一个进程可以同时打开的最大描述符的数量
    int i, maxindex, listenfd, connfd, sockfd;
    int nready; // poll 返回的符合条件描述符的数量
    struct sockaddr_in servaddr, clientaddr;
    socklen_t clientaddrlen;
    struct pollfd client[OPEN_MAX];
    ssize_t n;
    char buf[MAXLINE];

    open_max = sysconf(_SC_OPEN_MAX);
    LOG_I("open_max:%ld", open_max);
    
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    Listen(listenfd, LISTENQ);

    // client[0] save listenfd which become readable when new client connection
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    
    for (i = 1; i < OPEN_MAX; ++i) {
        client[i].fd = -1;
    }
    maxindex = 0;

    LOG_I("server start");
    for ( ; ; ) {
        // poll 输入 & 输出使用不同的参数(与 select 不同),因此不用每次循环都重新赋值
        nready = Poll(client, maxindex+1, INFTIM);
        if (client[0].revents & POLLRDNORM) { // new client connection add to list
            clientaddrlen = sizeof(clientaddr);
            connfd = Accept(listenfd, &clientaddr, &clientaddrlen);

            for (i = 1; i < OPEN_MAX; ++i) {
                if (client[i].fd < 0) {
                    LOG_I("new client connfd:%d", connfd);
                    client[i].fd = connfd;
                    break;
                }
            }
            if (i == OPEN_MAX) {
                err_quit("too many clients"); // 进程退出后之前连接的客户端也退出, 服务器关闭
            }

            client[i].events = POLLRDNORM;
            if (i > maxindex)
                maxindex = i;
            
            if(--nready <= 0) // no more readable descriptors
                continue;
        }

        for (i = 1; i <= maxindex; ++i) {
            if ((sockfd = client[i].fd) < 0)
                continue;
            if (client[i].revents & (POLLRDNORM | POLLERR)) {
                if ((n = read(sockfd, buf, MAXLINE)) < 0) { // connection reset by client
                    if (errno == ECONNRESET) {
                        Close(sockfd);
                        client[i].fd = -1;
                    } else {
                        err_sys("read error");
                    }
                } else if (n == 0) { /* connection closed by client */
                    Close(sockfd);
                    client[i].fd = -1;
                } else { // success
                    Write(sockfd, buf, n);
                }

                if(--nready <= 0) // no more readable descriptors
                    break;
            }
        } 
    }
    
}
#else
#error "not implement"
#endif