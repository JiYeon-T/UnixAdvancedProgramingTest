#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"


#define ECHO_SERV_PORT                  9999 // echo 服务测试端口
#define ECHO_UDP_SERVER                 0 // 精简版本
#define JUDGE_SERVER_ADDR               0 // 判断是否是服务器的返回
#define CONNECT_OVERLOAD_TEST           1 // POSIX 重载了 connect 函数用于 UDP
#define UDP_CLIENT_CONNECT              1 // UDP 客户端调用 connect 提升性能以及可以获取到错误
#define UDP_STREAM_CTRL_TEST            0 // UDP 发送流量控制测试



static void dg_client(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servaddrlen);
static void dg_client_v2(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servaddrlen);
static void dg_client_v3(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servaddrlen);
static void dg_client_v4(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen);


int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in servaddr, clientaddr;
    socklen_t seraddrlen, cliaddrlen;

    if (argc != 2)
        err_quit("usage: client <IP address>");

    sockfd = Socket(AF_INET, SOCK_DGRAM, 0); // UDP:datagram 数据报套接字

#if CONNECT_OVERLOAD_TEST
    // 1.在 UDP 套接字上调用 connect 并不会给对端主机发送任何消息,它完全是一个本地操作
    // 只是保存对端的 IP 地址以及端口号
    // 2. 在一个未绑定端口号的 UDP 套接字上调用 connect 同时也给该套接字指派一个临时端口
    // seraddrlen = sizeof(servaddr);
    // connect(sockfd, (const struct sockaddr*)&servaddr, &servaddr);
    // LOG_I("servaddr1:%s port:%d", sock_ntop(&servaddr, seraddrlen), ntohs(servaddr.sin_port));
#endif

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(argv[1]); // ??? 127.0.0.1
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

#if CONNECT_OVERLOAD_TEST
    seraddrlen = sizeof(servaddr);
    connect(sockfd, (const struct sockaddr*)&servaddr, &servaddr);
    LOG_I("servaddr2:%s port:%d", sock_ntop(&servaddr, seraddrlen), ntohs(servaddr.sin_port));
    // TODO:
    // localaddr 返回 NULL????
    cliaddrlen = sizeof(clientaddr);
    Getsockname(sockfd, &clientaddr, &cliaddrlen);
    LOG_I("local addr:%s", sock_ntop((const struct sockaddr*)&clientaddr, cliaddrlen));
    // Getpeername(sockfd, (const struct sockaddr*)&servaddr, &seraddrlen);
    // LOG_I("peer addr:%s", sock_ntop((const struct sockaddr*)&servaddr, seraddrlen));
#else

#endif

#if ECHO_UDP_SERVER
    dg_client(stdin, sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
#elif JUDGE_SERVER_ADDR
    dg_client_v2(stdin, sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
#elif UDP_CLIENT_CONNECT
    dg_client_v3(stdin, sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
#elif UDP_STREAM_CTRL_TEST
    dg_client_v4(stdin, sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
#else
    while (1);
#endif
}

/**
 * @brief 该函数是协议无关的
 *       问题:客户端没有判断服务器端信息, 所有发往该端口的数据都被认为是服务器的返回
 * 
*/
static void dg_client(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servaddrlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE + 1];

    while (Fgets(sendline, sizeof(sendline), fp) != NULL) {
        // NOTE: 对于 udp 套接字,客户端首次调用 sendto 时,内核会为该套接字分配一个端口
        // 和 tcp client 一样,客户可以显示调用 bind() 不过很少这样做:
        Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servaddrlen);
        // 不关心服务器端地址信息, 后两个参数可以填 NULL
        n = Recvfrom(sockfd, recvline, sizeof(recvline), 0, NULL, NULL);
        recvline[n] = '\0'; // null terminate
        Fputs(recvline, stdout);
    }
}

#if JUDGE_SERVER_ADDR
/**
 * @brief 通过返回的服务器地址信息判断是否是服务器的返回
 *      对于同一个主机拥有多个 IP 的情况会出现异常, 发送的 目的 IP 地址与收到的 IP 地址不同(但确实是同一台主机);
 *      解决方法:
 *      (1) recvfrom() 返回地址信息后通过 DNS 查看域名是否是自己需要的
 *      (2) UDP 服务端为每个 IP 地址创建一个 socket(), 然后用 select(), 保证服务器端用相同的 IP 地址返回
*/
static void dg_client_v2(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servaddrlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE + 1];
    struct sockaddr *p_reply_addr;
    struct sockaddr_in *serv_ipv4_addr; // ipv4
    socklen_t reply_addr_len;
    char addrbuff[MAXLINE];

    p_reply_addr = Malloc(sizeof(struct sockaddr));
    ASSERT(p_reply_addr != NULL);

    while (Fgets(sendline, sizeof(sendline), fp) != NULL) {
        Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servaddrlen);
        // 不关心服务器端地址信息, 后两个参数可以填 NULL
        reply_addr_len = servaddrlen; 
        n = Recvfrom(sockfd, recvline, sizeof(recvline), 0, p_reply_addr, &reply_addr_len);
        if (reply_addr_len != servaddrlen || 
            memcmp(p_reply_addr, pservaddr, servaddrlen) != 0) { // 判断服务器地址是否正确
            LOG_E("reply from %s ignored", sock_ntop(p_reply_addr, reply_addr_len));
            continue;
        } else {
            serv_ipv4_addr = (struct sockaddr_in *)p_reply_addr;
            Inet_ntop(AF_INET, (const void *)&serv_ipv4_addr->sin_addr, addrbuff, sizeof(addrbuff));
            LOG_I("recv udp server ip:%s port:%d", addrbuff, serv_ipv4_addr->sin_port);
        }

        recvline[n] = '\0'; // null terminate
        Fputs(recvline, stdout);
    }

    Free(p_reply_addr);
}
#endif

#if UDP_CLIENT_CONNECT
/**
 * @brief UDP 客户端调用 connect
 *        调用 connect 后可以收到 ICMP 不可达的消息, 但是是在 read 的时候, 而不是 connect 的时候
 *        ICMP unreachable 被内核映射成了 - ECONNREFUSED 错误
 *        tcpdump         
 * 这个接口仍然是协议无关的
 *        
 *    
*/
static void dg_client_v3(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servaddrlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE + 1];

    //NOTE:可以说 UDP 客户进程/服务器只在使用自己的 udp 套机诶子与确定的唯一对端进行通信时,才可以调用 connect
    //调用 connect 通常是 udp 客户端,不过有些网络应用中的 udp 服务器会与单个客户进行长时间通信(如:TFTP)
    //这种情况下客户和服务器都可以调用 connect
    n = connect(sockfd, pservaddr, servaddrlen);
    if (n < 0)
        err_ret("connect failed %s", strerror(errno));
    
    // Fgets - Linux 末尾多一个回车 \n, 有办法解决?
    while (Fgets(sendline, sizeof(sendline), stdin) != NULL) {
        // 当对 UDP 套接字调用 connect 后不能再像原来一样调用 sendto, 否则返回 EISCONN
        // 或者设置 sendto 的地址参数为 NULL 即可  
        // Write(sockfd, sendline, sizeof(sendline));
        // TODO:这里为什么不报错????????????????????
        // Sendto(sockfd, sendline, sizeof(sendline), 0, pservaddr, servaddrlen);
        // Sendto(sockfd, sendline, sizeof(sendline), 0, NULL, NULL);
        // LOG_I("%X %X %X", sendline[0], sendline[1], sendline[2]);
        LOG_D("input len:%d", strlen(sendline));
        n = Write(sockfd, sendline, strlen(sendline));
        LOG_D("write len:%d", n);
        n = Read(sockfd, recvline, sizeof(recvline));
        recvline[n] = '\0'; // null terminate
        LOG_D("recv3 %s", recvline);
        Fputs(recvline, stdout);
    }
}

/**
 * @brief 重复调用 connect 可以:1.指定新的 ip 地址&端口; 2.断开 udp 连接;
*/
static void disconnect_udp(int sockfd)
{
    // ipv4
    struct sockaddr_in addr;
    socklen_t servaddrlen = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_UNSPEC;

    connect(sockfd, (struct sockaddr *)&addr, servaddrlen);
}
#endif

#if UDP_STREAM_CTRL_TEST
/**
 * @brief UDP 缺乏流量控制
*/
static void dg_client_v4(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
#define     NDG         2000 //datagrams to send
#define     DGLEN       1400 //length of each datagram
    int i;
    char sendline[DGLEN];

    for (i = 0; i < NDG; ++i) {
        Sendto(sockfd, sendline, DGLEN, 0, pservaddr, servlen);
    }
    LOG_I("send %d packets finished.", NDG);
}
#endif