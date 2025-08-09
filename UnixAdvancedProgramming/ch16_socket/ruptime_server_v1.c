#include "../common/apue.h"
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>


// 面向连接的服务器
// 服务器读取 uptime 的输出,然后再发送到套接字



#define BUFLEN              128
#define QLEN                10

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
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    if ((err = getaddrinfo(host, "ruptime", &hint, &ailist)) != 0) {
        syslog(LOG_ERR, "ruptimed: getaddrinfo error:%s", gai_strerror(err));
        exit(1);
    }
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = init_server(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen, QLEN)) >= 0) {
            serve(sockfd);
            printf("exit\n");
            exit(0);
        }
    }
    exit(-1);
}

void serve(int sockfd)
{
    int clfd;
    FILE *fp;
    char buf[BUFLEN];

    // set_cloexec(sockfd);
    for (;;) {
        if ((clfd = accept(sockfd, NULL, NULL)) < 0) {
            syslog(LOG_ERR, "ruptimed:accept error:%s", strerror(errno));
            exit(1);
        }
        // set_cloexec(clfd);
        if ((fp = popen("/usr/bin/uptime", "r")) == NULL) {
            sprintf(buf, "error:%s\n", strerror(errno));
            send(clfd, buf, strlen(buf), 0);
        } else {
            while(fgets(buf, BUFLEN, fp) != NULL) 
                send(clfd, buf, strlen(buf), 0);
            printf("server:item:%s\n", buf);
            pclose(fp);
        }
        close(clfd);
    }
}
