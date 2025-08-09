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


#define UNIX_DOMAIN_STREAM_TEST     0 // TCP
#define UNIX_DOMAIN_DATAGRAM_TEST   1 // UDP

#define UNIX_DOMAIN_SOCKET_PATH     "/tmp/test"


static void dg_echo(int sockfd, struct sockaddr *clientaddr, socklen_t cliaddrlen);


int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_un servaddr, cliaddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

    unlink(UNIX_DOMAIN_SOCKET_PATH);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, UNIX_DOMAIN_SOCKET_PATH);

    Bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr));

    LOG_I("unix domain udp server start");
    dg_echo(sockfd, (const struct sockaddr*)&cliaddr, sizeof(cliaddr));
}

/**
 * @brief 该函数是协议无关的
 *        迭代服务器,因为 udp 是一个没有连接的协议,没有像 tcp 一样 EOF 之类的东西
 *        一般来说, tcp 服务器都是并发服务器,而 udp 都是迭代服务器
*/
static void dg_echo(int sockfd, struct sockaddr *clientaddr, socklen_t cliaddrlen)
{
    int n;
    socklen_t len;
    char msg[MAXLINE];
    struct sockaddr_in *client_addr;
    char buff[MAXLINE];

    for (; ;) {
        len = cliaddrlen;
        n = Recvfrom(sockfd, msg, sizeof(msg), 0, clientaddr, &len);
        client_addr = (struct sockaddr_in *)clientaddr;
        // Inet_ntop(AF_INET, (const void *)&client_addr->sin_addr, buff, sizeof(buff));
        // LOG_D("server recv from:%s p:%d", buff, htons(client_addr->sin_port));
        LOG_D("serv recv:%s", msg);
        Sendto(sockfd, msg, n, 0, clientaddr, len);
    }
}
