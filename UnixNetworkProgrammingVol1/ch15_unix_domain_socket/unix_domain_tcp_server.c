#include "../util/util.h"
#include "../util/unp.h"

#include <string.h> // memcpy
#include <strings.h> // bzero()
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <syslog.h>
#include <unistd.h>
#include <sys/un.h> // Unix domain socket


#define UNIX_DOMAIN_STREAM_TEST     1 // TCP
#define UNIX_DOMAIN_DATAGRAM_TEST   0 // UDP

#define UNIX_DOMAIN_SOCKET_PATH     "/tmp/test"


void sig_child_handler(int signo);
void str_echo(int sockfd);

pid_t pid;


/**
 * @brief TCP/数据流套接字, 提供没有边界的字节流服务
 * 
*/
int main(int argc, char *argv[])
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_un cliaddr, servaddr;

#if UNIX_DOMAIN_STREAM_TEST
    listenfd = Socket(AF_LOCAL, SOCK_STREAM, 0);
#endif
    unlink(UNIX_DOMAIN_SOCKET_PATH);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, UNIX_DOMAIN_SOCKET_PATH);

    Bind(listenfd, &servaddr, sizeof(servaddr));

    Listen(listenfd, LISTENQ);

    Signal(SIGCHLD, sig_child_handler);

    LOG_I("unix domain socket server start");
    for ( ; ; ) {
        clilen = sizeof(cliaddr);
        if ( (connfd = Accept(listenfd, (const struct sockaddr *)&cliaddr, &clilen)) < 0 ) {
            if (errno == EINTR)
                continue;
            else
                err_sys("accept error");
        }
        LOG_I("client [%d] connected", connfd);

        if ( (childpid = Fork()) == 0 ) { // child process
            Close(listenfd);
            str_echo(connfd);
            exit(0);
        } else {
            // LOG_I("client [%d] exit", connfd);
        }
        pid = childpid;
        Close(connfd);
    }
}

void sig_child_handler(int signo)
{
    int status;
    if (waitpid(pid, &status, 0) == pid) {
        LOG_I("process %d normally exit\n", pid);
    } else {
        LOG_E("process %d exit abnormal %s\n", pid, strerror(errno));
    }

}

void str_echo(int sockfd)
{
    uint8_t buff[MAXLINE];
    int n;

    for (; ;) {
        n = Read(sockfd, buff, sizeof(buff));
        if (n == 0 && errno == EINTR)
            continue;
        buff[n] = '\0';
        LOG_I("client [%d] recv:%s", sockfd, buff);
        Write(sockfd, buff, strlen(buff));
    }
}