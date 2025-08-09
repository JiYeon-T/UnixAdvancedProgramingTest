#include "../common/apue.h"
#include <stdint.h>

#include <arpa/inet.h>

#include <sys/socket.h>

#include <netdb.h> // 网络有关的配置信息

#if defined(SOLARIS)
#include <netinet/in.h>
#endif

#if defined(BSD)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
// 不同计算机进程间的通信方式 network IPC


/***************************************************************
 * Macro
 ***************************************************************/

/***************************************************************
 * Function Declaration
 ***************************************************************/
extern int socket(int domain, int type, int protocol);
extern int close(int fd);
extern int shutdown(int sockfd, int how);

extern uint32_t htonl(uint32_t hostint23); // 字节序转换
extern uint16_t htons(uint16_t hostint16);
extern uint32_t ntohl(uint32_t netint32);
extern uint16_t ntohs(uint16_t netint16);

extern in_addr_t inet_addr (const char *__cp); // IPv4 二进制地址和点分十进制(xx.xx.xxx.xx)转换
extern char *inet_ntoa (struct in_addr __in);
// extern char *inet_ntop(int domain, const void *restrict addr, char * restrict str, socklen_t size); // IPv6 & IPv4
extern int inet_pton(int domain, const char * restrict str, void * restrict addr);

extern struct hostent *gethostend(void); // 获取计算机系统的主机信息
extern void sethostent(int stayopen);
extern void endhostent(void);
extern struct hostent *gethostbyaddr (const void *__addr, __socklen_t __len,
				      int __type);
extern struct hostent *gethostbyname (const char *__name); 

extern struct netent *getnetent(void); // 获取网络名字和编号
extern void setnetent(int stayopen);
extern void endnetent(void);

extern struct protoent *getprotobyname(const char *name);
extern struct protoent *getprotobynumber(int proto);
extern struct protoent *getprotojent(void);
extern void setprotoent(int stayopen);
extern void endprotoent(void);

extern struct servent *getservbyname(const char *name, const char *proto);
extern struct servent *getservbyport(int port, const char *proto);
extern struct servent *getservent(void);
extern void setservent(int syayopen);
extern void endservent(void);

extern int getaddrinfo(const char *restrict host, const char *restrict service, 
        const struct addrinfo *restrict hint, struct addrinfo **restrict res);
extern const char *gai_strerror(int error);
extern int getnameinfo(const struct sockaddr * restrict addr, socklen_t alen, 
        char *restrict host, socklen_t hostlen, char *restrict service, socklen_t servlen, int flags);

// 网络字节序和点分十进制(127.0.0.7)的相互转换
// extern const char *inet_ntop(int domain, const void *restrict addr, char * restrict str, socklen_t size);
// extern const char *inet_pton(int domain, const char * restrict sttr, void * restrict addr);

extern struct hostent *gethostent(void); 
extern void sethostent(int stayopen);
extern void endhostent(void);

extern struct netent *getnetbyaddr(uint32_t net, int type);
extern struct netent *getnetbyname(const char *name);
extern struct netent *getnetent(void);
extern void setnetent(int stayopen);
extern void endnetent(void);

extern struct protoent *getprotobyname(const char *name);
extern struct protoent *getprotobynumber(int proto);
extern struct protoent *getprotoent(void);
extern void setprotoent(int stayopen);
extern void endprotoent(void);

extern struct servent *getservbyname(const char *name, const char *proto);
extern struct servent *getservbyport(int port, const char *proto);
extern struct  servent *getservent(void);
extern void setservent(int stayopen);
extern void endservent(void);

// extern int getaddrinfo(const char *restrict host, const char *restrict service, 
//                         struct addinfo *restrict hint, struct addrinfo **restrict res);
extern void freeaddrinfo(struct addrinfo *ai);
extern const char *gai_strerror(int error);
extern int getnameinfo(const struct sockaddr * restrict addr, socklen_t alen, char * restrict host,
                        socklen_t hostlen, char * restrict service, socklen_t servlen, int flags);

extern int bind(int sockfd, const struct sockaddr *addr, socklen_t len);
extern int gettsockname(int sockfd, struct sockaddr * restrict addr, socklen_t *restrict alenp);
extern int getpeername(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict alenp);
extern int connect(int sockdf, const struct sockaddr *addr, socklen_t len);
extern int listen(int sockfd, int backlog);
extern int accept(int sockfd, struct sockaddr * restrict addr, socklen_t * restrict len);

// 用于套接字的收发函数
ssize_t send(int sockfd, const void *buf, size_t nbytes, int flags);
ssize_t sendto(int sockfd, const void *buf, size_t nbytes, int flags, const struct sockaddr *destaddr, socklen_t destlen);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t recv(int sockfd, void *buf, size_t nbytes, int flags);
ssize_t recvfrom(int sockffd, void *restrict buf, size_t len, int flags, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);

// 套接字选项
extern int setsockopt(int sockfd, int level, int option, const void *val, socklen_t len);
extern int getsockopt(int sockfd, int level, int option, void * restrict val, socklen_t * restrict lenp);


/***************************************************************
 * Private Function Declaration
 ***************************************************************/
static void sequence_test(void);
/***************************************************************
 * Private Variables
 ***************************************************************/
static union {  
    uint32_t a;
    uint8_t c;
} test_var;
/***************************************************************
 * Public Variables
 ***************************************************************/
void print_family(struct addrinfo *aip)
{
    printf(" family:");
    switch (aip->ai_family) {
        case AF_INET:
            printf("inet");
            break;
        case AF_INET6:
            printf("inet6");
            break;
        case AF_UNIX:
            printf("unix");
            break;
        case AF_UNSPEC:
            printf("unspecified");
            break;
        default:
            printf("unknown");
            break;
    }
}

void print_type(struct addrinfo *aip)
{
    printf(" type:");
    switch (aip->ai_socktype) {
        case SOCK_STREAM:
            printf("stream");
            break;
        case SOCK_DGRAM:
            printf("datagram");
            break;
        case SOCK_SEQPACKET:
            printf("seqpacket");
            break;
        case SOCK_RAW:
            printf("raw");
            break;
        default:
            printf("unknown %d", aip->ai_socktype);
    }
}

void print_protocol(struct addrinfo *aip)
{
    printf(" protocol:");
    switch (aip->ai_socktype) {
        case 0:
            printf("default");
            break;
        case IPPROTO_TCP:
            printf("TCP");
            break;
        case IPPROTO_UDP:
            printf("UDP");
            break;
        case IPPROTO_RAW:
            printf("raw");
            break;
        default:
            printf("unknown %d", aip->ai_protocol);
            break;
    }
}

void print_flags(struct addrinfo *aip)
{
    printf(" flags:");
    if (aip->ai_flags == 0) {
        printf("0");
    } else {
        if (aip->ai_flags & AI_PASSIVE)
            printf(" passive ");
        if (aip->ai_flags & AI_CANONNAME)
            printf("canon ");
        if (aip->ai_flags & AI_NUMERICHOST)
            printf(" numhost ");
        if (aip->ai_flags & AI_NUMERICSERV)
            printf(" numserv ");
        if (aip->ai_flags & AI_V4MAPPED)
            printf(" v4mapped ");
        if (aip->ai_flags & AI_ALL)
            printf(" all ");
    }
}

// 字节序, 通常计算机端是小端但是网络中传输时是大端传输的
// 判断一台机器是大端还是小端的方法
static void sequence_test(void)
{
    test_var.a = 1;
    printf("c = %d\n", test_var.c); // 小端存储的
}

void network_ipc_test1(void)
{
    // sequence_test();

}

void get_addr_info_test1(int argc, char *argv[])
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    struct sockaddr_in *sinp;
    const char *addr;
    int err;
    char abuf[INET_ADDRSTRLEN];

    if (argc != 3)
        err_quit("usage: ./a.out nodename service");
    hint.ai_flags = AI_CANONNAME;
    hint.ai_family = 0;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    if ((err = getaddrinfo(argv[1], argv[2], &hint, &ailist)) != 0)
        err_ret("get addrinfo failed");
    for (aip = ailist; aip != NULL; aip = aip->ai_next) { // 遍历链表
        print_flags(aip);
        print_family(aip);
        print_type(aip);
        print_protocol(aip);
        printf("\n\thost %s ", aip->ai_canonname ? aip->ai_canonname : "-");
        if (aip->ai_family == AF_INET) {
            sinp = (struct sockaddr_in *)aip->ai_addr;
            addr = inet_ntop(AF_INET, &sinp->sin_addr, abuf, INET_ADDRSTRLEN);
            printf(" address %s", addr ? addr : "unknown");
            printf(" port %d", ntohs(sinp->sin_port));
        }
        printf("\n");

    }
}

/**
 * @fun connect 由于一些服务器的瞬时原因可能连接失败, 增加重连
 * @param[in] sockfd
 * @param[in] addr
 * @param[in] alen
 * @ret
*/
int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{
    #define MAXSLEEP    128
    int numsec;

    // 指数补偿算法
    for (numsec = 1; numsec <= MAXSLEEP; numsec << 1) {
        if (connect(sockfd, addr, alen) == 0) {
            // connect accepted
            return (0);
        }
        // delay before trying connect
        if (numsec <= MAXSLEEP / 2)
            sleep(numsec);
    }

    return (-1);
}

/**
 * @fun 为了兼容其他 UNIX 操作系统, eg:FreeBSD, Mac OS X
 *      connect 失败, 套接字的状态会变成未定义的
 *
 * @param[in] sockfd
 * @param[in] addr
 * @param[in] alen
 * @ret
*/
int connect_retry_v2(int domain, int type, int protocol, const struct sockaddr *addr, socklen_t alen)
{
    #define MAXSLEEP    128
    int numsec, fd;

    // 指数补偿算法
    for (numsec = 1; numsec <= MAXSLEEP; numsec << 1) {
        if ((fd = socket(domain, type, protocol)) < 0)
            return (-1);
        if (connect(fd, addr, alen) == 0) {
            // connect accepted
            printf("connect success fd:%d\n", fd);
            return fd;
        }
        close(fd); // 连接失败, 关闭 fd
        printf("connect failed:%d\n", fd);
        // delay before trying connect
        if (numsec <= MAXSLEEP / 2)
            sleep(numsec);
    }

    return (-1);
}

/**
 * @fun init server
 *
 * @param[in] sockfd
 * @param[in] addr
 * @param[in] alen
 * @ret
*/
int init_server(int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
    int fd;
    int err = 0;

    if ((fd = socket(addr->sa_family, type, 0)) < 0) {
        printf("error\n");
        return (-1);
    }
    if (bind(fd, addr, alen) < 0)
        goto errout;
    
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET) {
        if (listen(fd, qlen) < 0)
            goto errout;
    }
    return (fd);

errout:
    printf("errout\n");
    err = errno;
    close(fd);
    errno = err;
    return (-1);
}

// 通常情况下, TCP 的实现不允许绑定同一地址, 除非超时;
// 参数 SO_RECUSEADDR 可以绕过这个限制
int init_server_v2(int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
    int fd;
    int err = 0;
    int reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0) {
        printf("error\n");
        return (-1);
    }
    // setsockopt
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
        goto errout;
    if (bind(fd, addr, alen) < 0)
        goto errout;
    
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET) {
        if (listen(fd, qlen) < 0)
            goto errout;
    }
    return (fd);

errout:
    printf("errout\n");
    err = errno;
    close(fd);
    errno = err;
    return (-1);
}