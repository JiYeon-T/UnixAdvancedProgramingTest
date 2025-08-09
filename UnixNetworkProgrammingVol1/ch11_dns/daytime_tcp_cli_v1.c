
#include "../util/util.h"
#include "../util/unp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h> // memcpy
#include <strings.h> // bzero()



/**
 * @brief 使用 gethostbyname 和 getservbyname 获取要连接服务器的 ip 和 端口号重写 daytime 客户端程序
 * 
 * @note ./daytime_client localhost daytime
 *       ./daytime_client www.baidu.com daytime
*/
int main(int argc, char *argv[])
{
    int sockfd, n;
    char recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;
    struct in_addr **pptr;
    struct in_addr *inetaddrp[2];
    struct in_addr inetaddr;
    struct hostent *hp;
    struct servent *sp;
    
    if (argc != 3) {
        // e.g. daytimetcpcli localhost daytime
        err_quit("usage:daytimetcpcli <hostname> <service>");
    }
    if ( (hp = gethostbyname(argv[1])) == NULL ) {
        // 除了可以输入主机名字外, 直接输入服务器 ip 地址也是支持的, 兼容
        if (inet_aton(argv[1], &inetaddr) == 0) {
            err_quit("hostname errorfor %s:%s", argv[1], hstrerror(h_errno));
        } else {
            inetaddrp[0] = &inetaddr;
            inetaddrp[1] = NULL;
            pptr = inetaddrp;
        }
    } else { // 通过 dns 查找服务器 ip 成功
        pptr = (struct in_addr **)hp->h_addr_list;
    }
    LOG_I("get serv ip success");
    print_host_info(hp);

    if ( (sp = getservbyname(argv[2], "tcp")) == NULL ) {
        err_quit("getservbyname error for %s:%s", argv[2], hstrerror(h_errno));
    }
    LOG_I("get serv port success");
    print_service_info(sp);

    for ( ; *pptr != NULL; pptr++) { // 遍历 Ip 列表
        sockfd = Socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = sp->s_port; // 都是网络字节序
        memcpy(&servaddr.sin_addr, *pptr, sizeof(struct in_addr));
        if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == 0) {
            LOG_I("dayclient client connect success");
            break;
        }
        err_ret("connect error");
        close(sockfd);
    }

    if (*pptr == NULL) {
        err_quit("unable to connect");
    }

    while ( (n = Read(sockfd, recvline, sizeof(recvline))) > 0 ) {
        recvline[n] = '\0'; // 0, null terminate
        LOG_D("client recv len:%d", n);
        Fputs(recvline, stdout);
    }
    LOG_E("client exit n=%d", n);
    exit(0);

}