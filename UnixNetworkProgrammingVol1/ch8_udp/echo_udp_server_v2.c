#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"


#define ECHO_SERV_PORT                  9999 // echo 服务测试端口

static void sig_child_handler(int signo);
static void dg_echo(int sockfd);


/**
 * @brief 同时监听 TCP/UDP 且使用同一个端口(TCP/UDP 端口是独立的)
 *        TCP 使用子进程实现并发服务器
 *        UDP
*/
int main(int argc, char *argv[])
{
    int listenfd, connfd, udpfd, nready, maxfdp1;
    char msg[MAXLINE];
    pid_t childpid;
    fd_set rset, wset, excepset;
    ssize_t n;
    socklen_t len, servaddrlen, cliaddrlen;
    const int on = 1;
    struct sockaddr_in cliaddr, servaddr;

    // TCP socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    // 为了防止本主机上有其他程序在使用该端口
    // TCP & UDP 端口是独立的,虽然他们端口号一致
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    Bind(listenfd, (const struct sockaddr*)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

    // UDP socket
    udpfd = Socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    //Q&A
    //Q:udpfd 也需要绑定吗????????
    //A:需要,通过 bind 操作将套接字和 ip 以及端口号绑定起来 
    Bind(udpfd, (const struct sockaddr*)&servaddr, sizeof(servaddr));

    Signal(SIGCHLD, sig_child_handler);
    FD_ZERO(&rset);
    LOG_I("TCP & UDP server started");

    for (; ;) {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);
        maxfdp1 = max(listenfd, udpfd) + 1;
        if ((nready = Select(maxfdp1, &rset, NULL, NULL, NULL)) < 0){
            if (errno = EINTR) { // SIGCHLD interrupt select
                LOG_I("signal interrupt continue");
                continue;
            } else {
                err_sys("select error");
            }
        }

        if (FD_ISSET(listenfd, &rset)) { // 当监听套接字可读式,说明有新的连接发生
            cliaddrlen = sizeof(cliaddr);
            connfd = Accept(listenfd , (const struct sockaddr*)&cliaddr, &cliaddrlen);
            if ((childpid = Fork()) == 0) { // child process
                Close(listenfd);
                dg_echo(connfd); // process echo
                exit(0);
            }
            Close(connfd); // parent close connected socket
        }

        if (FD_ISSET(udpfd, &rset)) {
            cliaddrlen = sizeof(cliaddr);
            n = Recvfrom(udpfd, msg, sizeof(msg), 0, (const struct sockaddr*)&cliaddr, &cliaddrlen);
            LOG_D("udp recv:%s", msg);
            Sendto(udpfd, msg, n, 0, (const struct sockaddr*)&cliaddr, cliaddrlen);
        }
    }
}

/**
 * @brief 
 * @note  子进程推出后对进程资源进行回收,避免成为僵尸进程
*/
static void sig_child_handler(int signo)
{
    pid_t pid;
    int exit_status;

    while ((pid = waitpid(-1, &exit_status, WNOHANG)) > 0) {
        printf("child %d terminated\n\n", pid);
        if (WIFEXITED(exit_status)) { // 打印进程退出状态
            printf("[%d] exited, status = %d\n", pid, WEXITSTATUS(exit_status));
        } else if (WIFSIGNALED(exit_status)) {
            printf("[%d] killed by signal, status = %d\n", pid, WEXITSTATUS(exit_status));
        } else if (WIFSTOPPED(exit_status)) {
            printf("[%d] stopped by signal, status = %d\n", pid, WEXITSTATUS(exit_status));
        } else if (WIFCONTINUED(exit_status)) {
            printf("[%d] continued, status=%d", pid, WEXITSTATUS(exit_status));
        }
    }
}

static void dg_echo(int sockfd)
{
    ssize_t n;
    char buff[MAXLINE];

again:
    while ((n = read(sockfd, buff, sizeof(buff))) > 0) {
        LOG_D("server %d recv[%d] %s", getpid(), n, buff);
        Writen(sockfd, buff, n);
    }

    if (n < 0 && errno == EINTR) // 系统调用被信号中断
        goto again;
    else if (n < 0)
        err_sys("server read error");
    else { // n == 0, 客户端正常退出

    }
}