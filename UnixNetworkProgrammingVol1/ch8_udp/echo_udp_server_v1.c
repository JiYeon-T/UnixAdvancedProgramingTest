#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"


#define ECHO_SERV_PORT                  9999 // echo 服务测试端口
#define UDP_STREAM_CTRL_TEST            1 // UDP 发送流量控制测试


static void dg_echo(int sockfd, struct sockaddr *clientaddr, socklen_t len);
static void sig_int_handler(int signo);


static long long recv_count = 0;

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in servaddr, clientaddr;
    int n;

    sockfd = Socket(AF_INET, SOCK_DGRAM, 0); // UDP:datagram 数据报套接字

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    Bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    LOG_I("UDP echo server started");

    recv_count = 0;
    Signal(SIGINT, sig_int_handler);

    // 增大接收缓冲区并不能解决 UDP 缺乏流量控制导致的丢包问题
    n = 220 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));

    dg_echo(sockfd, (struct sockaddr*)&clientaddr, sizeof(clientaddr));
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
        recv_count += n;
        client_addr = (struct sockaddr_in *)clientaddr;
        Inet_ntop(AF_INET, (const void *)&client_addr->sin_addr, buff, sizeof(buff));
        LOG_D("server recv from:%s p:%d", buff, htons(client_addr->sin_port));
        LOG_D("serv recv:%s", msg);
        Sendto(sockfd, msg, n, 0, clientaddr, len);
    }
}

// UDP 缺乏流量控制, 这里打印接收到的总包数
static void sig_int_handler(int signo)
{
    if (signo == SIGINT) {
        LOG_I("received %d bytes", recv_count);
        exit(0);
    }
}