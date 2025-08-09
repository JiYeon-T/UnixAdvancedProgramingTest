#include "../common/apue.h"
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>

// 面向连接的客户端

// Q&A
// Q:Servname not supported for ai_socktype
// A:出现这种错误的原因是因为没有给服务分配端口号，可以手动添加端口号，
// 就是在/etc/services文件里加上一行：使用vi /ect/services
// 按住Shift+G跳到最后一行，按住i进入插入模式，进行编辑 ruptime 39001/tcp
// ruptime 是服务名，就是getaddrinfo的第二个参数名，而不是程序名。
// 39001是分配的端口号，可以任意，但不要与其他服务的一样，还有就是不能小于1024。
// 因为1024以下，是为了给系统服务预留的，不要占用。 tcp就是协议名。

#define BUFLEN          128

extern int connect_retry_v2(int domain, int type, int protocol, const struct sockaddr *addr, socklen_t alen);
void print_uptime(int sockfd);


int main(int argc, char *argv[])
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err;

    if (argc != 2)
        err_quit("usage:ruptime hostname");
    
    memset(&hint, 0, sizeof(hint));
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    if ((err = getaddrinfo(argv[1], "ruptime", &hint, &ailist)) != 0)
        err_quit("getaddrinfo error:%s", gai_strerror(err));
    
    // 如果服务器支持多重网络接口或多重网络协议, getaddrinfo() 会返回多个候选地址供使用
    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sockfd = connect_retry_v2(aip->ai_family, SOCK_STREAM, 0, aip->ai_addr, aip->ai_addrlen)) < 0) {
            err = errno;
        } else {
            print_uptime(sockfd);
            exit(0);
        }
    }
    err_exit("can't connect tot %s", argv[1]);
}

void print_uptime(int sockfd)
{
    int n;
    char buf[BUFLEN];

    while ((n = recv(sockfd, buf, BUFLEN, 0)) > 0)
        write(STDOUT_FILENO, buf, n);
    if (n < 0)
        err_sys("recv error");
}

