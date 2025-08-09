#include "../common/apue.h"
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>


// 面向连接的服务器
// 服务器读取 uptime 的输出,然后再发送到套接字



#define BUFLEN              128
#define MAXADDRLEN          256

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX       256
#endif

extern int init_server(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
void serve(int sockfd);


int main(int argc, char *argv[])
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err, n;
    char *host;

    if (argc != 1)
        err_quit("usage ruptimed");
    if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0)
        n = HOST_NAME_MAX;
    if ((host = malloc(n)) == NULL)
        err_sys("malloc error");
    if (gethostname(host, n) < 0)
        err_sys("gethostname error");
    else
        printf("host name:%s\n", host);
    
    // daemonize("ruptimed");
    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_CANONNAME;
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(host, "ruptime", &hint, &ailist)) != 0) {
        syslog(LOG_ERR, "ruptimed: getaddrinfo error:%s", gai_strerror(err));
        printf("error:%s\n", gai_strerror(err));
        exit(1);
    }
    printf("1\n");

    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = init_server(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen, 0)) >= 0) {
            serve(sockfd);
            printf("exit\n");
            exit(0);
        }
        printf("addr:%s\n", (char*)aip->ai_addr);
    }
    printf("exit -1");
    exit(-1);
}

void serve(int sockfd)
{
    int n;
    socklen_t alen;
    int clfd;
    FILE *fp;
    char buf[BUFLEN];
    char addrbuf[MAXADDRLEN];
    struct sockaddr *addr = (struct sockaddr *)addrbuf;

    // set_cloexec(sockfd);
    for (;;) {
        alen = MAXADDRLEN;
        if ((n = recvfrom(sockfd, buf, BUFLEN, 0, addr, &alen)) < 0) {
            syslog(LOG_ERR, "ruptimed: recv from error:%s\n", strerror(errno));
            exit(1);
        }
        if ((fp = popen("/usr/bin/uptime", "r")) == NULL) {
            sprintf(buf, "error:%s\n", strerror(errno));
            sendto(sockfd, buf, strlen(buf), 0, addr, alen);
            pclose(fp);
        }
    }
}
