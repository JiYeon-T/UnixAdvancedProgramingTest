#include "../util/util.h"
#include "../util/unp.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <string.h> // memcpy
#include <strings.h> // bzero()

#include <arpa/inet.h>

// #include "sum.h"
/***************************************************************
 * MACRO
 ***************************************************************/

/***************************************************************
 * GLOBAL FUNCTION DECLARATION
 ***************************************************************/
// 主机字节序与网络字节序的转换函数
extern uint16_t htons(uint16_t host16bitvalue);
extern uint32_t htonl(uint32_t host32bitvalue);
extern uint16_t ntohs(uint16_t net16bitvalue);
extern uint32_t ntohl(uint32_t net32bitvalue);
// 二进制数表示的 ipv4 地址与 可读 ASCII 字符串之间的转换,仅用于 ipv4 
extern int inet_aton(const char *strptr, struct in_addr *addrptr);
extern in_addr_t inet_addr(const char *strptr); // 已废弃,无法处理多播地址:255.255.255.255
extern char *inet_ntoa(struct in_addr inaddr); // 不可重入
// 后续新增的支持 ipv4 & ipv6 的地址格式转换接口, p-presentation, n-numeric
extern int inet_pton(int family, const char *strptr, void *addrptr);
// extern const char* inet_ntop(int family, const void *addrptr, char *strptr, size_t len);


// 字节操纵函数
void bzero(void *dest, size_t nbytes);
void bcopy(const void *src, void *dest, size_t nbytes); // 注意顺序相反
int bcmp(const void *ptr1, const void *ptr2, size_t nbytes);

void *memset(void *dest, int c, size_t len);
void *memcpy(void *dest, const void *src, size_t nbytes);
int memcmp(const void *ptr1, const void *ptr2, size_t nbytes);

/***************************************************************
 * PRIVATE FUNCTION DECLARATION
 ***************************************************************/

/***************************************************************
 * PRIVATE FUNCTION
 ***************************************************************/
/**
 * @brief 判断本机的字节序(大小端)
 * 
 * @ret 1:big endian, 0:little endian
*/
static const char* host_seq1(void)
{
    union {
        char a;
        int b;
    } var;
    var.b = 1;
    printf("a:%d b:%d\n", var.a, var.b);

    printf("host seq:%s\n", var.a == 1 ? "little endian" : "big endian");
}

/**
 * @brief 判断主机字节序2
 * 
 * @ret 1:big endian, 0:little endian
*/
static void host_seq(void)
{
    union {
        short s;
        char c[sizeof(short)];
    } un;
    un.s = 0x0102;
    // printf("%s:", CPU_VERDOR_OS); // TODO:???
    if (sizeof(short) == 2) {
        if (un.c[0] == 1 && un.c[1] == 2) {
            printf("big-endian\n");
        } else if (un.c[0] = 2 && un.c[1] == 1) {
            printf("little-endian\n");
        } else {
            printf("unknown");
        }
    } else {
        printf("sizeof(short)=%lu\n", sizeof(short));
    }
}

/**
 * @brief 二进制保存的 Ipv4 地址与可阅读的点分十进制字符串之间的转换
 * 网络字节序:大端
 * 主机字节序:不固定
*/
static void ipv4_addr_print_test(void)
{
    const char *addr = "127.0.0.1";
    struct in_addr inaddr;

    inet_aton(addr, &inaddr); // 弃用
    printf("int addr:%X\n", inaddr.s_addr);
    
    bzero(&inaddr, sizeof(inaddr));
    addr = NULL;
    inaddr.s_addr = 0x10000001;
    addr = inet_ntoa(inaddr); // 弃用
    printf("addr str:%s\n", addr);

    printf("\nlocalhost:%x\n", inet_addr("127.0.0.1"));
    printf("broadcat:%x\n", inet_addr("255.255.255.255"));
    
}

/**
 * @brief new address format
 * 网络字节序:大端
*/
static void ipv6_addr_format_test(void)
{
    char ipv4_addr[INET_ADDRSTRLEN];
    char ipv6_addr[INET6_ADDRSTRLEN];
    struct in_addr inaddr; // member `sin_addr` of `struct sockaddr_in`
    struct in6_addr in6addr; // member of `struct sockaddr_in6`

    strncpy(ipv4_addr, "127.0.0.1", INET_ADDRSTRLEN);
    strncpy(ipv6_addr, "fe80::f4c1:4486:7c8d:edfb", INET6_ADDRSTRLEN);

    printf("ipv4 addr:%s\n", ipv4_addr);
    printf("ipv6_addr:%s\n", ipv6_addr);
    if (inet_pton(AF_INET, ipv4_addr, &inaddr) != 1) // 成功返回1, 失败返回0/-1
        err_ret("inet_pton 4 failed");
    printf("int addr:0x%X\n\n", inaddr.s_addr);
    if (inet_pton(AF_INET6, ipv6_addr, &in6addr) != 1)
        err_ret("inet_pton 6 failed");

    bzero(ipv4_addr, sizeof(ipv4_addr));
    bzero(ipv6_addr, sizeof(ipv6_addr));
    if (inet_ntop(AF_INET, &inaddr, ipv4_addr, sizeof(ipv4_addr)) == NULL)
        err_ret("inet_ntop 4 failed");
    if (inet_ntop(AF_INET6, &in6addr, ipv6_addr, sizeof(ipv6_addr)) == NULL)
        err_ret("inet_ntop 6 failed");
    printf("ipv4 addr:%s\n", ipv4_addr);
    printf("ipv6_addr:%s\n", ipv6_addr);
}

/**
 * @brief 仅支持 ipv4 地址转换 inet_pton 函数的一个简单实现
*/
static int inet_pton_m(int family, const char *strptr, void *addrptr)
{
    if (family == AF_INET) {
        struct in_addr in_val;
        if (inet_aton(strptr, &in_val)) {
            memcpy(addrptr, &in_val, sizeof(struct in_addr));
            return (1);
        }
        return (0);
    }

    errno = EAFNOSUPPORT;
    return (-1);
}

/**
 * @brief 仅支持 ipv4 地址转换 inet_ntop 函数的一个简单实现
*/
static char * inet_ntop_m(int family, const void *addrptr, char *strptr, size_t len)
{
    const u_char *p = (const u_char *)addrptr;

    if (family == AF_INET) {
        char temp[INET_ADDRSTRLEN];
        snprintf(temp, sizeof(temp), "%d.%d.%d.%d", p[0], p[1], p[2], p[3]); // uin32_t -> 点分十进制写法
        if (strlen(temp) >= len) {
            errno = ENOSPC;
            return (NULL);
        }
        strcpy(strptr, temp);
        return (strptr);
    }

    errno = EAFNOSUPPORT;
    return (NULL);
}

static void ipv4_addr_format_test_m(void)
{
    char ipv4_addr[INET_ADDRSTRLEN];
    struct in_addr inaddr; // member `sin_addr` of `struct sockaddr_in`

    strncpy(ipv4_addr, "127.0.0.1", INET_ADDRSTRLEN);

    printf("ipv4 addr:%s\n", ipv4_addr);
    if (inet_pton_m(AF_INET, ipv4_addr, &inaddr) != 1) // 成功返回1, 失败返回0/-1
        err_ret("inet_pton 4 failed");
    printf("int addr:0x%X\n\n", inaddr.s_addr);

    bzero(ipv4_addr, sizeof(ipv4_addr));
    if (inet_ntop_m(AF_INET, &inaddr, ipv4_addr, sizeof(ipv4_addr)) == NULL)
        err_ret("inet_ntop 4 failed");

    printf("ipv4 addr:%s\n", ipv4_addr);

    return;
}
/***************************************************************
 * PUBLIC FUNCTION
 ***************************************************************/
#ifdef IPV4_TEST
/**
 * tcp daytime client
 * ipv4
*/
int main(int argc, char *argv[])
{
    int sockfd, n;
    char recvline[MAXLINE + 1];
    // 套接字地址结构说明:
    // 大多数套接字函数都需要一个指向套接字地址结构的指针作为参数.
    // 每个协议族都定义它自己的套接字地址结构.
    // 这些地址结构均以 sockaddr_ 开头, 并以对应每个协议族唯一的后缀
    // e.g. ipv4:sockaddr_in, 
    // ipv6:sockaddr_in6, 
    // unix domain socket:sockaddr_un
    // TODO:
    // 新的通用的地址结构: struct sockaddr_storage 如何使用??
    struct sockaddr_in servaddr;

    if (argc != 2)
        err_quit("usage: a.out <IPadress>");
    
    /**
     * AF_INET Ipv4 协议
     * AF_INET6 ipv6 协议
     * AF_LOCAL Unix域协议 TODO:ch15
     * AF_ROUTE 路由套接字 TODO:ch18
     * AF_KEY 密钥套接字 TODO:ch19
    */
    if ((sockfd = Socket(AF_INET, SOCK_STREAM, 0)) < 0) // TCP socket
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr)); // 按照惯例,我们总是在填写前把整个结构置零, 而不仅仅是吧 sin_zero 字段置零
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13); // 转换字节序:host to network short(endian)
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) // convert IPv4 and IPv6 addresses from text to binary form
        err_quit("inet_pton fail argv[1]:%s", argv[1]);
    
    // param2: servaddr 必须强转为通用套接字地址结构类型:struct sockaddr
    // param3: 套接字地址结构的长度 socklen_t 类型
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        // connect() 函数将当前套接字从 CLOSED 状态转移到 SYN_SENT 状态, 若成功, 转移到 ESTABLISHED 状态
        // 若失败, 则该套接字不再可用, 必须关闭 close(), 我们对这样的套接字继续使用
        // 因此, 每次 connect() 失败后, 都必须关闭当前 socket 并创建新的 socket()
        Close(sockfd);
        err_quit("connect error");
    }
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = '\0';
        if (fputs(recvline, stdout) == EOF)
            err_quit("fputs error");
        LOG_D("recv:%s", recvline);
    }

    if (n < 0)
        err_sys("read error");

    exit(0);
}
#elif defined(IPV6_TEST)
/**
 * tcp daytime client
 * ipv6
 * TODO: ipv6 service & client running error
*/
int main(int argc, char *argv[])
{
    int sockfd, n;
    char recvline[MAXLINE + 1];
    struct sockaddr_in6 servaddr;

    if (argc != 2)
        err_quit("usage: a.out <IPadress>");
    
    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) // TCP socket
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(13); // 转换字节序:host to network short(endian)
    if (inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) <= 0) // convert IPv4 and IPv6 addresses from text to binary form
        err_quit("inet_pton fail argv[1]:%s", argv[1]);
    
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        err_quit("connect error");
    
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = '\0';
        if (fputs(recvline, stdout) == EOF)
            err_quit("fputs error");
    }

    if (n < 0)
        err_sys("read error");

    exit(0);
}
#else
int main(int argc, char *argv[])
{
    // host_seq1();
    // host_seq();
    ipv4_addr_print_test();
    // ipv6_addr_format_test();
    TIME_MEASURE_START();
    ipv4_addr_format_test_m();
    long long ns;
    TIME_MEASURE_END(ns);
    printf("timeusage:%llu ns\n", ns);
}
#endif