#include "unp.h"


extern int socket(int family, int type, int protocol);
extern int connect(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen);
extern int listen(int sockfd, int backlog);
extern int accept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen);

extern int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern int getsockname(int sockfd, struct sockaddr *localaddr, socklen_t *addrlen);
extern int getpeername(int sockfd, struct sockaddr *peeraddr, socklen_t *addrlen);

extern int poll(struct pollfd *fdarray, unsigned long nfds, int timeout);
 
// 设置套接字选项
extern int getsockopt(int sockfd, int level, int optname,
                      void *optval, socklen_t *optlen);
extern int setsockopt(int sockfd, int level, int optname,
                const void *optval, socklen_t optlen);
extern int fcntl(int fd, int cmd, ... /* arg */ ); // file control
extern int ioctl(int fd, unsigned long request, ...);
// UDP
extern ssize_t recv(int sockfd, void *buf, size_t len, int flags);

extern ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);

extern ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
extern ssize_t send(int sockfd, const void *buf, size_t len, int flags);

extern ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                const struct sockaddr *dest_addr, socklen_t addrlen);

extern ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
//SCTP
extern int sctp_bindx(int sd, struct sockaddr * addrs, int addrcnt,
                      int flags);
extern int sctp_connectx(int sd, struct sockaddr * addrs, int addrcnt,
                    sctp_assoc_t  * id);
extern int sctp_getpaddrs(int sd, sctp_assoc_t assoc_id, // 该函数会分配内存, 需要使用 sctp_freepaddrs 释放
                        struct sockaddr **addrs);
// extern void sctp_freepaddrs(struct sockaddr *addrs);
extern int sctp_getladdrs(int sd, sctp_assoc_t assoc_id, // 获取某个关联的本地地址, 该函数分配的内存需要使用 sctp_freeladdrs() 释放
                          struct sockaddr **addrs);
// extern void sctp_freeladdrs(struct sockaddr *addrs);
extern int sctp_sendmsg(int sd, const void * msg, size_t len,
                        struct sockaddr *to, socklen_t tolen,
                        uint32_t ppid, uint32_t flags,
                        uint16_t stream_no, uint32_t timetolive,
                        uint32_t context);
extern int sctp_recvmsg(int sd, void * msg, size_t len,
                        struct sockaddr * from, socklen_t * fromlen,
                        struct sctp_sndrcvinfo * sinfo, int * msg_flags);
extern int sctp_opt_info(int sd, sctp_assoc_t id, int opt,
                         void * arg, socklen_t * size);
extern int sctp_peeloff(int sd, sctp_assoc_t assoc_id);

/***************************************************************
 * Network
 ***************************************************************/
// TODO:
// 为什么这里一加声明就报错??? 函数声明是直接拷贝的啊
// static char *sock_str_flag(union val *p_val, int len);
// static char *sock_str_int(union val *p_val, int len);
// static char *sock_str_linger(union val *p_val, int len);
// static char *sock_str_timeval(union val *p_val, int len);

union val { // 保存 getsockopt() 所有可能返回的数据类型
    int i_val;
    long l_val;
    struct linger linger_val;
    struct timeval timeval_val;
} val;

struct sock_opts {
    const char *opt_str;
    int opt_level;
    int opt_name; // defined at tcp/ip stack include files
    char *(*print_opt_val_str)(union val *p_val, int len); // 打印选项字符串
};
static char strres[128];



/**
 * @brief 定义包裹函数来减少代码行数, 出错就退出程序
 * 
 * @param[in] faimily AF_INET:ipv4; AF_INET6:ipv6; AF_LOCAL:Unix domain socket; 
 *                      AF_ROUTE :路由套接字; AF_KEY:密钥套接字
 * @param[in] type SOCK_STREAM:字节流套接字; SOCK_DGRAM:数据报套接字; SOCK_SEQPACKET:有序分组套接字;
 *                 SOCK_RAW: 原始套接字
 * @param[in] protocol IPPROTO_CP:TCP; IPPROTO_UDP:UDP; IPPROTO_SCTP:SCTP
*/
int Socket(int family, int type, int protocol)
{
    int n;

    if ((n = socket(family, type, protocol)) < 0)
        err_sys("socket err %s", strerror(errno));
    
    return (n);
}

/**
 * @brief 将一个威廉解的套接字转换成一个被动套接字
 * 
 * @param[in] sockfd
 * @param[in] backlog 内核为相应套接字排队的最大连接个数
*/
int Listen(int sockfd, int backlog)
{
    int n;
    char *ptr;

    // 通过环境变量来控制连接队列的大小, 从而保证修改大小时不需要重新编译服务器
    if ((ptr =getenv("LISTENQ")) != NULL)
        backlog = atoi(ptr);

    if ((n = listen(sockfd, backlog)) < 0)
        err_sys("listen err %s", strerror(errno));

    return (n);
}

/**
 * @brief 把一个协议地址赋予一个套接字
 * 
 * @param[in] sockfd
 * @param[in] addr 特定于协议地址结构的指针
 *                 对于 TCP, 可以指定一个端口号, 或指定一个 IP 地址, 或二者都指定, 或二者都不指定
 *                 如果都不指定, 调用 Listen/connect 时内核就会为 socket 分配一个临时端口
 *                 对于 TCP 客户端来说, 都不指定是正常的.但是对于服务器来说, 是不正常的,因为服务器需要一个众所周知的端口被大家认识.
 * @param[in] addrlen
*/
int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int n;

    if ((n = bind(sockfd, addr, addrlen)) < 0)
        err_sys("bind err %s", strerror(errno));

    return (n);
}

/**
 * @brief 由 TCP 服务器低矮用, 用于从已完成连接队列头返回下一个已完成连接的 socket
 *        如果已完成连接队列为空, 进程进入睡眠
 * 
 * @param[in] sockfd
 * @param[in] addr 
 * @param[in] addrlen
*/
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int n;

    if ((n = accept(sockfd, addr, addrlen)) < 0)
        err_sys("accept err %s", strerror(errno));

    return (n);
}


/**
 * @brief 
 *        客户端在 connect() 前不必非得调用 bind(), 因为如果需要的化内核会确定源 IP 地址,
 *        并选择一个临时端口作为源端口号
*/
#if 0 // TODO: 这里为什么编译报错???
int Connect(int sockfd, const struct scokaddr *addr, socklen_t addrlen)
{
    int n;

    if ((n = connect(sockfd, addr, addrlen)) < 0)
        err_sys("accept err %s", strerror(errno));

    return (n);
}
#endif

/**
 * @brief network to prensentation
*/
const char *Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    char *ret;

    if ((ret = inet_ntop(af, src, dst, size)) == NULL) {
        err_sys("inet_ntop err %s", strerror(errno));
    }

    return ret;
}

/**
 * @brief prensentation to network
*/
int Inet_pton(int af, const char *src, void *dst)
{
    char *ret;

    if ((ret = inet_pton(af, src, dst)) == NULL) {
        err_sys("inet_pton err %s", strerror(errno));
    }

    return ret;
}

/**
 * @brief 获取套接字的地址族
*/
int sockfd_to_family(int sockfd)
{
    struct sockaddr_storage ss; // sockaddr_storage 可以存储任何套接字地址结构
    socklen_t len;

    len = sizeof(ss);
    if (Getsockname(sockfd, (struct sockaddr *)&ss, &len) < 0)
        return (-1);

    return (ss.ss_family);
}

// TODO: 怎么获取地址信息
#if 0
void print_socket_info(int sockfd)
{
    struct sockaddr_storage ss;
    socklen_t len;

    len = sizeof(ss);
    if ((getsockname(sockfd, &ss, &len)) < 0)
        return;
    LOG_I("len:%d family:%d addr:%s port:%d", len, ss.ss_family, )
}

void print_peer_info(int sockfd)
{
    struct sockaddr_storage ss;
    socklen_t len;

    len = sizeof(ss);
    if ((getpeername(sockfd, &ss, &len)) < 0)
        return;
    LOG_I("len:%d family:%d addr:%s port:%d", len, ss.ss_family, )

    return;
}
#endif

/**
 * @brief 关闭套接字
 *        1. 不管引用计数是否为 0, 都关闭套接字, 触发四次挥手流程
 *        2. socket:全双工套接字, 通过 howto 决定关闭读半部还是写半部
 * @param[in] sockfd
 * @param[in] howto
*/
int Shutdown(int sockfd, int howto)
{
    int n;

    if ((n = shutdown(sockfd, howto)) < 0)
        err_sys("accept err %s", strerror(errno));

    return (n);

}
/***************************************************************
 * Socket Configuration
 ***************************************************************/
/**
 * @brief 用于确认功能是否开启, ON/OFF
*/
static char *sock_str_flag(union val *p_val, int len)
{
    if (len != sizeof(int)) {
        snprintf(strres, sizeof(strres), "size %d not sizeof(int)", len);
    } else {
        snprintf(strres, sizeof(strres), "%s", p_val->i_val == 0 ? "OFF" : "ON");
    }

    return (strres);
}

/**
 * @brief 打印 int 数值
*/
static char *sock_str_int(union val *p_val, int len)
{
    if (len != sizeof(int)) {
        snprintf(strres, sizeof(strres), "size %d not sizeof(int)", len);
    } else {
        snprintf(strres, sizeof(strres), "%d", p_val->i_val);
    }

    return (strres);
}

/**
 * @brief 打印 long 数值
*/
static char *sock_str_long(union val *p_val, int len)
{
    if (len != sizeof(long)) {
        snprintf(strres, sizeof(strres), "size %d not sizeof(long)", len);
    } else {
        snprintf(strres, sizeof(strres), "%ld", p_val->l_val);
    }

    return (strres);
}

/**
 * @brief 打印 struct linger 数值
*/
static char *sock_str_linger(union val *p_val, int len)
{
    struct linger *p = p_val;

    if (len != sizeof(struct linger)) {
        snprintf(strres, sizeof(strres), "size %d not sizeof(linger)", len);
    } else {
        snprintf(strres, sizeof(strres), "linger:%s time:%d", 
                p->l_linger ? "ON" : "OFF", p->l_onoff);
    }

    return (strres);
}

static char *sock_str_timeval(union val *p_val, int len)
{
    struct timeval *p = p_val;

    if (len != sizeof(struct timeval)) {
        snprintf(strres, sizeof(strres), "size %d not sizeof(linger)", len);
    } else {
        snprintf(strres, sizeof(strres), "%ld us", p->tv_sec * 1000000 + p->tv_usec);
    }

    return (strres);
}

static struct sock_opts s_sock_ops[] = {
    {"SO_BROADCAST",    SOL_SOCKET,     SO_BROADCAST,   sock_str_flag},
    {"SO_DEBUG",        SOL_SOCKET,     SO_DEBUG,       sock_str_flag},
    {"SO_DONTROUTE",    SOL_SOCKET,     SO_DONTROUTE,   sock_str_flag},
    {"SO_ERROR",        SOL_SOCKET,     SO_ERROR,       sock_str_int},
    {"SO_KEEPALIVE",    SOL_SOCKET,     SO_KEEPALIVE,   sock_str_flag},
    {"SO_LINGER",       SOL_SOCKET,     SO_LINGER,      sock_str_linger},
    {"SO_OOBINLINE",    SOL_SOCKET,     SO_OOBINLINE,   sock_str_flag},
    {"SO_RCVBUF",       SOL_SOCKET,     SO_RCVBUF,      sock_str_int},
    {"SO_SNDBUF",       SOL_SOCKET,     SO_SNDBUF,      sock_str_int},
    {"SO_RCVLOWAT",     SOL_SOCKET,     SO_RCVLOWAT,    sock_str_int},
    {"SO_SNDLOWAT",     SOL_SOCKET,     SO_SNDLOWAT,    sock_str_int},
    {"SO_RCVTIMEO",     SOL_SOCKET,     SO_RCVTIMEO,    sock_str_timeval},
    {"SO_SNDTIMEO",     SOL_SOCKET,     SO_SNDTIMEO,    sock_str_timeval},
    {"SO_REUSEADDR",    SOL_SOCKET,     SO_REUSEADDR,   sock_str_flag},
#ifdef SO_REUSEPORT
    {"SO_REUSEPORT",    SOL_SOCKET,     SO_REUSEPORT,   sock_str_flag},
#else
    {"SO_REUSEPORT",    0,              0,              NULL},
#endif
    {"SO_TYPE",         SOL_SOCKET,     SO_TYPE,        sock_str_int},
    // {"SO_USELOOPBACK",  SOL_SOCKET,     SO_USELOOPBACK, sock_str_flag},
    
    {"IP_TOS",          IPPROTO_IP,     IP_TOS,         sock_str_int},
    {"IP_TTL",          IPPROTO_IP,     IP_TTL,         sock_str_int},

    {"IPV6_DONTFRAG",   IPPROTO_IPV6,   IPV6_DONTFRAG,  sock_str_flag},
    {"IPV6_UNICAST_HOPS",IPPROTO_IPV6,  IPV6_UNICAST_HOPS,sock_str_int},
    {"IPV6_V6ONLY",     IPPROTO_IPV6,   IPV6_V6ONLY,    sock_str_flag},

    {"TCP_MAXSEG",      IPPROTO_TCP,     TCP_MAXSEG,    sock_str_int},
    {"TCP_NODELAY",     IPPROTO_TCP,     TCP_NODELAY,   sock_str_flag},

    {"SO_TYPE",         IPPROTO_SCTP,    SCTP_AUTOCLOSE,sock_str_int},
    {"SCTP_MAX_BURST",  IPPROTO_SCTP,    SCTP_MAX_BURST,sock_str_int},
    {"SCTP_MAXSEG",     IPPROTO_SCTP,    SCTP_MAXSEG,   sock_str_int},
    {"SCTP_NODELAY",    IPPROTO_SCTP,    SCTP_NODELAY,  sock_str_flag},

    // other configurations

    {NULL,              0,               0,             NULL}
};

void print_socket_config(void)
{
    struct sock_opts *ptr;
    int fd;
    socklen_t len;
    union val val;

    for (ptr = s_sock_ops; ptr->print_opt_val_str != NULL; ++ptr) {
        printf("%s  ", ptr->opt_str);
        if (ptr->print_opt_val_str == NULL) {
            printf("undefined\n");
            continue;
        }
        switch (ptr->opt_level) {
            case SOL_SOCKET:
            case IPPROTO_IP:
            case IPPROTO_TCP:
                fd = Socket(AF_INET, SOCK_STREAM, 0);
                break;
// #if defined(IPV6)
            case IPPROTO_IPV6:
                fd = Socket(AF_INET6, SOCK_STREAM, 0);
                break;
// #endif
#ifdef IPPROTO_SCTP
            case IPPROTO_SCTP:
                fd = Socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
                break;
#endif
        default:
            err_quit("can't create fd for level %d", ptr->opt_level);
        }

        len = sizeof(val);
        if (getsockopt(fd, ptr->opt_level, ptr->opt_name, &val, &len) == -1) {
            err_ret("getsockopt failed");
        } else {
            printf("default = %s\n", (*ptr->print_opt_val_str)(&val, len));
        }
        Close(fd);
    }
}

int Getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int n;

    if ((n = getsockname(sockfd, addr, addrlen)) < 0)
        err_sys("getsockname failed %s", strerror(errno));
    
    return n;
}

int Getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int n;

    if ((n = getpeername(sockfd, addr, addrlen)) < 0)
        err_sys("getpeername failed %s", strerror(errno));
    
    return n;
}
/***************************************************************
 * Thread Oper ation
 ***************************************************************/
void Pthread_mutex_lock(pthread_mutex_t *mptr)
{
    int n;

    if ((n = pthread_mutex_lock(mptr)) == 0)
        return;
    errno = n;
    err_sys("pthread_mutex_lock error");
}

pid_t Fork(void)
{
    pid_t n;

    if ((n = fork()) < 0)
        err_sys("fork err %s", strerror(errno));
    
    return n;
}

/**
 * @brief 强烈建议使用 sigaction
 *      signal 函数不同平台实现不同, 因为它的出现早于 posix
 * 
 * @ret 旧信号处理函数
*/
SigFunc *Signal(int signo, SigFunc *func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask); // 在该信号处理函数运行期间, 不阻塞额外的信号
    act.sa_flags = 0;

    if (signo == SIGALRM) { // SIG_ALRAM 通常用于打断阻塞的系统调用(read等),因此,不使能 SA_RESTART
#ifdef SA_INTERRUPT
// #error "1"
        // 与 SA_RESTART 互补,即不会重启系统调用
        act.sa_flags |= SA_INTERRUPT; // SunOS 4.x
#endif
    } else {
#ifdef SA_RESTART
// #error "2"
        // NOTE:被信号中断的系统调用将由内核自动重启
        // 由于有些平台不支持 SA_RESTART 或者对该信号不支持,因此在不同的平台上系统调用仍然会被捕获
        // 中断,用户需要判断 EINTR 错误,然后继续开始
        // TODO:除了 connect, 如果 connect 被中断,不可以重新执行,必须调用 select 等待连接完成
        act.sa_flags |= SA_RESTART; // SVR4 4.4BSD
#endif
    }

    if (sigaction(signo, &act, &oact) < 0)
        return (SIG_ERR);
    
    return oact.sa_handler;
}

/**
 * @brief  On success, select() and pselect() return the number of  file  descrip‐
 *    tors  contained  in  the  three  returned descriptor sets 
*/
int Select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout)
{
    int n;

    if ((n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0)
        err_sys("select err %s", strerror(errno));

    return (n);
}

int Poll(struct pollfd *fdarray, unsigned long nfds, int timeout)
{
    int n;

    if ((n = poll(fdarray, nfds, timeout)) < 0)
        err_sys("poll err %s", strerror(errno));

    return (n);
}

int Pselect(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, const struct timespec *timeout,
                   const sigset_t *sigmask)
{
    int n;

    if ((n = pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask) ) < 0) {
        err_sys("select err %s", strerror(errno));
    }

    return (n);
}

/***************************************************************
 * File Operation
 ***************************************************************/
int Write(int fd, const void *buf, size_t count)
{
    int n;

    if ((n = write(fd, buf, count)) < 0)
        err_sys("write err %d %s", errno, strerror(errno));

    return (n);
}

ssize_t Read(int fd, void *buf, size_t count)
{
    int n;

    if ((n = read(fd, buf, count)) < 0)
        err_sys("read err %s", strerror(errno));

    return (n);
}

/**
 * @brief 多进程使用该描述符时, close 仅导致 "引用计数" -1, 而不会关闭套接字, 
 *        如果要关闭, 可以使用 shutdown()
*/
int Close(int fd)
{
    int n;

    if ((n = close(fd) ) < 0)
        err_sys("close err %s", strerror(errno));

    return (n);
}

/**
 * @brief 自己实现的地址转换函数, 与协议无关, 兼容 Ipv4 & ipv6
 *        存在局部静态变量, 不可重入, 非线程安全
*/
char *sock_ntop(const struct sockaddr *sa, socklen_t addrlen)
{
    char portstr[8];
    static char addr_str[256]; // unix domain is largest
    struct sockaddr_in *sin;
    struct sockaddr_in6 *sin6;

    bzero(addr_str, sizeof(addr_str));

    switch (sa->sa_family) {
        case AF_INET: {
            sin = (struct sockaddr_in *)sa;
            if (inet_ntop(AF_INET, &sin->sin_addr, addr_str, sizeof(addr_str)) == NULL)
                return (NULL);
            if (ntohs(sin->sin_port) != 0) {
                snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
                strcat(addr_str, portstr);
                return (addr_str);
            }
            break;
        }
        case AF_INET6:
            sin6 = (struct sockaddr_in6*)sa;
            if (inet_ntop(AF_INET6, &sin6->sin6_addr, addr_str, sizeof(addr_str)) == NULL)
                return NULL;
            if (ntohs(sin6->sin6_port) != 0) {
                snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin6->sin6_port));
                strcat(addr_str, portstr);
                return (addr_str);
            }
            break;
        default:
        break;
    }

    return (NULL);
}

// TODO:
/**
 * @brief 将通配地址和一个临时端口绑定到一个套接字
*/
int sock_bind_wild(int sockfd, int family)
{
    return (0);
}

/**
 * @brief 比较两个套接字地址结构的地址
*/
int sock_cmp_addr(const struct sockaddr *sockaddr1, 
                    const struct sockaddr *sockaddr2, socklen_t addrlen)
{
    return (0);

}

/**
 * @brief 比较两个套接字地址结构的端口
*/
int sock_cmp_port(const struct sockaddr *sockaddr1, 
                    const struct sockaddr *sockaddr2, socklen_t addrlen)
{
    return (0);
}

/**
 * @brief 返回套接字地址结构的端口号
*/
int sock_get_port(const struct sockaddr *sockaddr, socklen_t addrlen)
{
    return (0);
}

/**
 * @brief numberic -> prensation
 * 把一个套接字地址结构中的主机部分转换成表达式格式
*/
char *sock_ntop_host(const struct sockaddr *sockaddr, socklen_t addrlen, void *ptr)
{
    // TODO:
    return (NULL);
}

/**
 * @brief 地址部分设置为 ptr 指向的内容
*/
void sock_set_addr(const struct sockaddr *sockaddr, socklen_t addrlen, void *ptr)
{

}

/**
 * @brief set port
*/
void sock_set_port(const struct sockaddr *sockaddr, socklen_t addrlen, int port)
{

}

/**
 * @brief 套接字地址设置为通配地址
*/
void sock_set_wild(const struct sockaddr *sockaddr, socklen_t addrlen)
{

}

/**
 * @brief 字节流套接字上进行读操作时,读到的数据量可能比想要读的少, 
 *       因为内核中套接字的缓冲区已经到达了极限
 *       此时需要调用者再次调用一次 read() 才可以读出剩余数据
 *       从管道/终端读取数据时都会出现该现象
 * 
*/
ssize_t Readn(int fd, void *vptr, size_t n)
{
    size_t nleft; // size_t unsigned long int
    ssize_t nread; // ssize_t - long int;
    char *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            /* EINTR 表示系统调用被一个捕获的信号中断,
            如果发生该错误则仅需进行读/写操作.既然这些函数的作用是避免让调用者来处理
            不足的字节计数值,那么我们就处理该错误,而不是强迫调用者再次调用 readn */
            if (errno == EINTR)
                nread = 0; // call read() again
            else
                return (-1);
        } else if (nread == 0) { // EOF
            break;
        }
        nleft -= nread; // 正常读取
        ptr += nread;
    }

    return (n - nleft);
}

ssize_t Writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR) // call write() again
                nwritten = 0;
            else // ERROR
                return (-1);
        }
        nleft -= nwritten;
        ptr += nwritten;
    }

    // return (n);
    return (n - nleft);
}

/**
 * @brief painfully slow version - example only
 *        call read() again when read a char
*/
ssize_t readline_v1(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; ++n) {
    again:
        if ((rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) { // EOF
            *ptr = 0;
            return (n - 1);
        } else {
            if (errno == EINTR) // again
                goto again;
            return -1;
        }
    }

    *ptr = 0;
    return (n);
}

/**
 * @brief use a buff to save data, increase effienicy
 *  use stdio's buffer is not good, because stdio's buffer is unpredictable
 * 使用标准库的 fread() 函数, 该函数有内部 buffer, 可以使用, 
 * 但是该 buffer 对于上层来说是不可以预知的, 最好不要使用, 而是要自己实现 buffer
*/
ssize_t readline_v2(int fd, void *vptr, size_t maxlen)
{

}

static int read_cnt;
static char *read_ptr;
static char read_buf[MAXLINE];

/**
 * @brief use a buff to save data, increase effienicy
 *        自己定义一个有缓存的 read 函数(避免了频繁执行系统调用 read()), 一次仅返回一个 byte
 *        缺点, 不再是线程安全的函数
*/
static ssize_t my_read(int fd, char *ptr)
{
    if (read_cnt <= 0) {
        again:
            if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) { // read fail
                if (errno == EINTR)
                    goto again;
            } else if (read_cnt == 0) { // EOF
                return (0);
            }
            read_ptr = read_buf; // 读取到了新的一块数据
    }

    read_cnt--;
    *ptr = *read_ptr++;

    return (1);
}

/**
 * @brief 使用自己实现的 my_read
 *        这个新函数能够暴露内部缓冲区的状态, 便于调用者查看在当前文本行之后是否收到了新的数据
*/
ssize_t readline_v3(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; ++n) {
        if ((rc = my_read(fd, &c)) == 1) { // read success
            *ptr = c;
            if (c == '\n') // new line is stored, like fgets()
                break;
        } else if (rc == 0) { // EOF, n-1 bytes was read
            *ptr = 0;
            return (n - 1);
        } else { // ERROR, errno set by read()
            return (-1);
        }
    }

    *ptr = 0; // null terminate like fgets()

    return (n);
}

/**
 * @brief 获取自己定义的 buff
*/
ssize_t readlinebuf_v3(void **vptrptr)
{
    if (read_cnt)
        *vptrptr = read_ptr;
    return (read_cnt);
}

char * Fgets(char *s, int size, FILE *stream)
{
    char *ret;

    ret = fgets(s, size, stream);

    return ret;
}

int Fputs(const char *s, FILE *stream)
{
    int ret;

    ret = fputs(s, stream);

    return ret;
}

/**
 * @brief 仅能用户属性是 bool 类型时使用
*/
bool get_fl(int fd, int flags)
{
    int val;

    if ((val = fcntl(fd, F_GETFL, 0)) < 0) {
        err_quit("fcntl F_GETFL failed");
    }

    return (val & flags);
}

/**
 * @fun: 设置文件状态标志
 *       必须要先读, 然后按位或, 然后写入,
 *       例如：打开标准输出流同步写标志 set_fl(STDOUT_FILENO, O_SYNC);
*/
void set_fl(int fd, int flags)
{
    int val;

    if((val = fcntl(fd, F_GETFL, 0)) < 0) {
        err_quit("fcntl F_GETFL failed");
    }

    val |= flags;

    if(fcntl(fd, F_SETFL, val) < 0) {
        err_sys("fcntl F_SETFL failed");
    }

    return;
}

/**
 * @fun: clear 某一个标志位的正确操作
 *       其他位不变, 仅将当前为清零
 *       必须要先读, 然后按位或, 然后写入,
*/
void clear_fl(int fd, int flags)
{
    int val;

    if((val = fcntl(fd, F_GETFL, 0)) < 0) {
        err_quit("fcntl F_GETFL failed");
    }

    val &= ~flags;

    if(fcntl(fd, F_SETFL, val) < 0) {
        err_sys("fcntl F_SETFL failed");
    }

    return;
}

ssize_t Recv(int sockfd, void *buf, size_t len, int flags)
{
    int n;

    if ( (n = recv(sockfd, buf, len, flags)) < 0)
        err_sys("recv error %s", strerror(errno));
    
    return n;
}

ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags,
                struct sockaddr *src_addr, socklen_t *addrlen)
{
    int n;

    if ( (n = recvfrom(sockfd, buf, len, flags, src_addr, addrlen)) < 0)
        err_sys("recvfrom error %s", strerror(errno));
    
    return n;
}
ssize_t Recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    int n;

    if ( (n = recvmsg(sockfd, msg, flags)) < 0)
        err_sys("recvmsg error %s", strerror(errno));
    
    return n;
}

ssize_t Send(int sockfd, const void *buf, size_t len, int flags)
{
    int n;

    if ( (n = send(sockfd, buf, len, flags)) < 0)
        err_sys("send error %s", strerror(errno));
    
    return n;
}
ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags,
                const struct sockaddr *dest_addr, socklen_t addrlen)
{
    int n;

    if ( (n = sendto(sockfd, buf, len, flags, dest_addr, addrlen)) < 0)
        err_sys("recvmsg error %s", strerror(errno));
    
    return n;
}

ssize_t Sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
    int n;

    if ( (n = sendmsg(sockfd, msg, flags)) < 0)
        err_sys("sendmsg error %s", strerror(errno));
    
    return n;
}

int Getsockopt(int sockfd, int level, int optname,
                void *optval, socklen_t *optlen)
{
    int n;

    if ( (n = getsockopt(sockfd, level, optname, optval, optlen)) < 0)
        err_sys("getsockopt error %s", strerror(errno));
    
    return n;
}

int Setsockopt(int sockfd, int level, int optname,
                const void *optval, socklen_t optlen)
{
    int n;

    if ( (n = setsockopt(sockfd, level, optname, optval, optlen)) < 0)
        err_sys("setsockopt error %s", strerror(errno));
    
    return n;
}

/**
 * @brief
 * @note
 * 
 * @param[in]  msg_flags  收到的数据类型,如通知:MSG_NOTIFICATION, MSG_EOR 等
*/
int Sctp_recvmsg(int sd, void * msg, size_t len,
                        struct sockaddr * from, socklen_t * fromlen,
                        struct sctp_sndrcvinfo * sinfo, int * msg_flags)
{
    int n;

    if ( (n = sctp_recvmsg(sd, msg, len, from, fromlen, sinfo, msg_flags) ) < 0)
        err_sys("sctp_recvmsg error %s", strerror(errno));

    return n;
}

/**
 * @brief
 * @note  sendmsg 函数也可以用于发送 sctp 数据,但是在辅助数据中填充比较麻烦
 *        这种方法比分配必要的辅助数据空间 并在 msghdr 结构中设置合适的结构(sendmsg 函数)容易些
 *        如果实现把 sctp_sendmsg 映射成  sendmsg, 那么 sendmsg 的 flags 参数设置为 0
 * 
 * @param[in] sd 
 * @param[in] msg
 * @param[in] len
 * @param[in] to  内容将发送到对端断点 to
 * @param[in] tolen to 的大小
 * @param[in] ppid  指定将随数据块传递的 payload 协议标识符
 * @param[in] flags 传递给 sctp 栈
 * @param[in] stream_no 指定一个sctp 流号
 * @param[in] timetolive 指定消息的生命周期,0:无线生命期, 单位:ms
 * @param[in] context 指定可能有的用户上下文, 用户上下文把通过消息通知机制收到的某次失败的消息发送 
 *                    与某个特定于应用的本地上下文联系起来
 * @param[in] 
*/
int Sctp_sendmsg(int sd, const void * msg, size_t len,
                        struct sockaddr *to, socklen_t tolen,
                        uint32_t ppid, uint32_t flags,
                        uint16_t stream_no, uint32_t timetolive,
                        uint32_t context)
{
    int n;

    if ( (n = sctp_sendmsg(sd, msg, len, to, tolen, ppid, flags, stream_no, timetolive,
                        context) ) < 0)
        err_sys("sctp_sendmsg error %s", strerror(errno));

    return n;
}

/**
 * @brief 获取 sctp 外出流数目
 * @note  为了便于移植,调用者应该使用 sctp_opt_info() 而不是使用 getsockopt()
*/
int sctp_get_no_strms(int sd, const struct sockaddr *addr, socklen_t len)
{
    int n;
    // NOTE:其中还有很多与套接字有关的选项可以使用
    struct sctp_status status;
    sctp_assoc_t assoc_id;
    socklen_t status_len;

    // TODO: 怎么获取到 assoc_id ???
    status.sstat_assoc_id = assoc_id;
    len = sizeof(status);
    if ((n = sctp_opt_info(sd, assoc_id, SCTP_STATUS, &status, &status_len)) < 0) {
        err_quit("sctp_opt_info error %s", strerror(errno));
    }

    return status.sstat_outstrms;
}

/**
 * @brief print sctp information
 * 
 * @param[in]  info struct sctp_sndrcvinfo
 * @retval
*/
void sctp_print_sctp_info(const void *info)
{
    if (!info)
        return;

    const struct sctp_sndrcvinfo *p = (const struct sctp_sndrcvinfo *)info;
    
    LOG_I("sinfo_stream:%d", p->sinfo_stream);
    LOG_I("sinfo_ssn:%d", p->sinfo_ssn);
    LOG_I("sinfo_flags:%d", p->sinfo_flags);
    LOG_I("sinfo_ppid:%d", p->sinfo_ppid);
    LOG_I("sinfo_context:%d", p->sinfo_context);
    LOG_I("sinfo_timetolive:%d", p->sinfo_timetolive);
    LOG_I("sinfo_tsn:%d", p->sinfo_tsn);
    LOG_I("sinfo_cumtsn:%d", p->sinfo_cumtsn);
    LOG_I("sinfo_assoc_id:%d", p->sinfo_assoc_id);
    printf("\r\n");
}

/**
 * @brief print sctp notification, 
 *       用户数据和"通知"将在套接字缓冲区中交替出现
 *        这里仅仅打印不同类型的字符串,每种类型对应的输入输出流等其他信息,还需要单独打印
 *
 * @param[in]  notif
 * @retval
*/
void sctp_print_notification(const void *notif_buf)
{
    union sctp_notification *snp;
    struct sctp_assoc_change *sac;
    struct sctp_paddr_change *spc;
    struct sctp_remote_error *sre;
    struct sctp_send_failed *ssf;
    struct sctp_shutdown_event *sse;
    struct sctp_adaptation_event *ae;
    struct sctp_pdapi_event *pdapi;
    struct sctp_authkey_event *sae;
    struct sctp_sender_dry_event *ssde;
    const char *str;

    snp = (union sctp_notification*)notif_buf;
    switch (snp->sn_header.sn_type) {
        // 关联本身发生变动
        case SCTP_ASSOC_CHANGE: {
            sac = &snp->sn_assoc_change;
            switch (sac->sac_state) {
                case SCTP_COMM_UP:
                    str = "COMMUNICATION UP";
                    break;
                case SCTP_COMM_LOST:
                    str = "COMMUNICATION LOST";
                    break;
                case SCTP_RESTART:
                    str = "REMOTE RESTART";
                    break;
                case SCTP_SHUTDOWN_COMP:
                    str = "SHUTDOWN COMPLETE";
                    break;
                case SCTP_CANT_STR_ASSOC:
                    str = "CAN'T START ASSOC";
                    break;
                default:
                    str = "UNKNOWN";
                    break;
            }
            LOG_I("SCTP_ASSOC_CHANGE: %s, assoc=0x%x", str, (uint32_t)sac->sac_assoc_id);
            break;
        }
        // 对端的某个地址经历了变动,这种变动即可以是失败性质,也可以是恢复性质
        case SCTP_PEER_ADDR_CHANGE: {
            spc = &snp->sn_paddr_change;
            switch (spc->spc_state) {
                case SCTP_ADDR_AVAILABLE:
                    str = "ADDRESS AVAILABLE";
                    break;
                case SCTP_ADDR_UNREACHABLE:
                    str = "ADDRESS UNREACHABLE";
                    break;
                case SCTP_ADDR_REMOVED:
                    str = "ADDRESS REMOVED";
                    break;
                case SCTP_ADDR_ADDED:
                    str = "ADDRESS ADDED";
                    break;
                case SCTP_ADDR_MADE_PRIM:
                    str = "ADDRESS MADE PRIMARY";
                    break;
                default:
                    str = "UNKNOWN";
                    break;
            }
            LOG_I("SCTP_PEER_ADDR_CHANGE: %s, addr=%s, assoc=0x%x", 
                str, 
                sock_ntop((struct sockaddr*)&spc->spc_aaddr, sizeof(spc->spc_aaddr)),
                (uint32_t)spc->spc_assoc_id);
            break;
        }
        // 远程错误,则显示该错误和发生它的关联 ID
        case SCTP_REMOTE_ERROR: {
            sre = &snp->sn_remote_error;
            LOG_I("SCTP_REMOTE_ERROR: assoc=0x%x errno=%d", (uint32_t)sre->sre_assoc_id,
                    sre->sre_error);
            break;
        }
        // 消息未能发送到对端
        case SCTP_SEND_FAILED: {
            ssf = &snp->sn_send_failed;
            LOG_I("SCTP_SEND_FAILED: assoc=0x%x error=%d", (uint32_t)ssf->ssf_assoc_id, ssf->ssf_error);
            break;
        }
        // 适应层指示参数:
        // 外出适应层使用 SCTP_ADAPTION_LAYER 套接字选项设置
        // TODO:
        case SCTP_ADAPTATION_INDICATION: {
            ae = &snp->sn_adaptation_event;
            LOG_I("SCTP_ADAPTATION_INDICATION: assoc=0x%x, adapt:%d", (uint32_t)ae->sai_assoc_id,
                (uint32_t)ae->sai_adaptation_ind);
            break;
        }
        // 部分递送应用程序接口用于 经由套接字缓冲区向用户传递大消息
        case SCTP_PARTIAL_DELIVERY_EVENT: {
            pdapi = &snp->sn_adaptation_event;
            LOG_I("SCTP_PARTIAL_DELIVERY_EVENT: assoc=0x%x", (uint32_t)pdapi->pdapi_assoc_id);
            break;
        }
        // 对端发送了一个 shutdown 消息,不再接受新的数据
        case SCTP_SHUTDOWN_EVENT: {
            sse = &snp->sn_shutdown_event;
            LOG_I("SCTP_SHUTDOWN_EVENT: assoc=0x%x", (uint32_t)sse->sse_assoc_id);
            break;
        }
        /*
        * 5.3.1.8.  SCTP_AUTHENTICATION_EVENT
        *
        *  When a receiver is using authentication this message will provide
        *  notifications regarding new keys being made active as well as errors.
        */
        case SCTP_AUTHENTICATION_INDICATION: {
            sae = &snp->sn_authkey_event;
            LOG_I("SCTP_AUTHENTICATION_INDICATION assoc=0x%x auth_type:%d", (uint32_t)sae->auth_assoc_id,
                    sae->auth_type);
            break;
        }
        case SCTP_SENDER_DRY_EVENT: {
            ssde = &snp->sn_sender_dry_event;
            LOG_I("SCTP_SENDER_DRY_EVENT assoc=0x%x type:%d", (uint32_t)ssde->sender_dry_assoc_id,
                    ssde->sender_dry_type);
            break;
        }
        default:
            LOG_I("Unknown notification event type:%d", snp->sn_header.sn_type);
            break;
    }
}
/* Domain Name System */
/**
 * @brief only return ipv4 address
 *        通过 DNS 服务查询
*/
struct hostent *Gethostbyname(const char *hostname)
{
    struct hostent *ph;

    if ( (ph = gethostbyname(hostname)) == NULL )
        err_sys("gethostbyname error:%s", hstrerror(h_errno));
    
    return ph;
}

// struct hostent *gethostbyaddr(const char *addr, socklen_t len, int family);
// extern struct hostent *gethostbyaddr (const void *__addr, __socklen_t __len,
// 				      int __type);
/**
 * @brief 通过服务名字获取端口号, 仅仅支持 ipv4
 *        查询本机文件 cat /etc/services
 *        
*/
struct servent *Getservbyname (const char *servname, const char *protoname)
{
    struct servent *ps;

    if ( (ps = getservbyname(servname, protoname)) == NULL )
        err_sys("getservbyname error:%s", hstrerror(h_errno));

}
extern struct servent *getservbyport (int port, const char *protoname);

/**
 * @brief 打印 hostent (主机)结构体信息
 *        
 * 
 * @param[in]  sptr
*/
void print_host_info(struct hostent *hptr)
{
    char *ptr, **pptr;
    char str[INET_ADDRSTRLEN];

    printf("host name:%s\n", hptr->h_name);

    for (pptr = hptr->h_aliases; *pptr != NULL; pptr++) // alias list
        printf("\talias:%s\n", *pptr);
    
    switch (hptr->h_addrtype) {
        case AF_INET: {
            pptr = hptr->h_addr_list;
            for (; *pptr != NULL; pptr++) { // alias address list
                printf("\taddress:%s\n", Inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
            }
            break;
        default:
            err_ret("unknown adress type");
            break;
        }
    }
    printf("\n");

    return;
}

/**
 * @brief 打印 servent (服务)结构体信息
 *        
 * 
 * @param[in]  sptr
*/
void print_service_info(struct servent *sptr)
{
    char *palias;
    int i = 0;

    if (!sptr) {
        printf("failed %s\n\n", hstrerror(h_errno));
        return;
    }
    printf("service name:%s\n", sptr->s_name);
    // TODO: 死循环
    // palias = *sptr->s_aliases;
    i = 0;
    palias = sptr->s_aliases[i];
    while (palias) {
        printf("alias name:%s\n", sptr->s_name);
        palias = sptr->s_aliases[++i];
    }
    printf("port:%d\n", ntohs(sptr->s_port));
    printf("protocol:%s\n\n", sptr->s_proto);    
}

/**
 * @brief 打印 netent (网络)结构体信息
 *        
 * 
 * @param[in]  sptr
*/
void print_network_info(struct netent *nptr)
{
    char *palias;
    int i;

    if (!nptr) {
        printf("failed %s\n\n", hstrerror(h_errno));
        return;
    }

    printf("network name:%s\n", nptr->n_name);
    i = 0;
    palias = nptr->n_aliases[i];
    while (palias != NULL) {
        printf("alias name:%s\n", palias);
        palias = nptr->n_aliases[++i];
    }
    printf("num:%d\n\n", nptr->n_net);
}

/**
 * @brief 打印 protoent (协议)结构体信息
 *        
 * 
 * @param[in]  sptr
*/
void print_protocol_info(struct protoent *pptr)
{
    char *palias;
    int i;

    if (!pptr) {
        printf("failed %s\n\n", hstrerror(h_errno));
        return;
    }
    printf("protocol name:%s\n", pptr->p_name);
    i = 0;
    palias = pptr->p_aliases[i];
    while (palias != NULL) {
        printf("alias name:%s\n", palias);
        palias = pptr->p_aliases[++i];
    }
    printf("num:%d\n\n", pptr->p_proto);
}

/**
 * @brief 可以同时处理名字到地址以及服务到端口这两种转换 
 *        getaddrinfo 把协议相关性完全隐藏在函数内部, 用到的所有内存都是动态分配的,需要调用相应的接口进行释放
 *        可以同时处理 Ipv4 & ipv6
 *        g.g. getaddrinfo("freebsd4", "domain", &hints, &res);
 *
 * @param[in]  hostname 主机名或地址串(ipv4的点分十进制串或 ipv6 的十六进制数串)
 * @param[in]  service 一个服务名或十进制端口号数串
 * @param[in]  hints 想要的结果暗示,可以是一个空指针,例如:如果指定的服务器既支持 tcp 也支持 udp(某个 DNS 服务器的 domain 服务)
 *                   当调用者设置 hints->ai_socktype 为 SOCK_DGRAM 时,使得返回的仅仅是适用于数据报套接字的信息
 *              ai_flags:
 *              AI_PASSIVE 套接字将用与被动打开
 *              AI_CANONNAME 告知 getaddrinfo 返回规范名字
 *              AI_NUMERICHOST 防止任何类型的名字到地址映射, hostnmae 必须是一个十进制串
 *              AI_NUMBERICSERV 防止任何类型的名字到服务映射, service 参数必须是一个十进制的端口号
 *              AI_V4MAPPED 如果同时指定 ai_family 成员的值为 AF_INET6, 那么如果没有可用的 AAAA
 *                          记录,就返回 A 记录对应的 ipv4 映射的 ipv6 地址
 *              AI_ALL 如果同时指定 AI_V4MAPPED, 那么除了返回与 AAAA 记录对应的 ipv6 地址外还返回
 *                          A 记录对应的 ipv4 映射的 ipv6 地址
 *              AI_ADDRCONFIG 按照所在主机的配置返回地址类型,也就是只查找所在主机回馈接口以外的网络接口配置的
 *               IP 地址版本一致的地址, TODO: ????
 *              ai_sockettype
 *              ai_protocol
 *             如果客户清楚自己只处理一种类型的套接字(例如 telnet 和 ftp 仅支持 tcp, tftp 仅支持 udp)
 *             那么可以把 hints->ai_socktype 成员设置成 SOCK_STREAM 或 SOCK_DGRM
 * @param[out] result 返回 addrinfo 类型的链表
 * @retval
 * */ 
int Getaddrinfo (const char * hostname,
			const char * service,
			const struct addrinfo * hints,
			struct addrinfo ** result)
{
    int n;

    if ( (n = getaddrinfo(hostname, service, hints, result)) < 0)
        err_sys("getaddrinfo error %s", hstrerror(h_errno));

    return n;
}

/**
 * @brief getaddrinfo 的异常返回值字符串
 *        EAI_AGAIN, EAI_BADFLAGS, EAI_FAIL, EAI_FAMILY, EAI_MEMORY, EAI_NONAME, EAI_OVERFLOW
 *        EAI_SERVICE, EAI_SOCKTYPE, EAI_SYSTEM
 * 
 * @param[in]  error
*/
const char *Gai_strerror(int error)
{
    return gai_strerror(errno);
}

/**
 * @brief getaddrinfo 中的指针成员指向的内存都是动态分配的,用完后要释放掉
 *        当需要拷贝 addrinfo 结构中的成员时,不可以直接拷贝结构体,需要分别拷贝成员
 *        浅拷贝 -> 深拷贝
 * 
 * @param[in]  ai
*/
void FreeAddrinfo(struct addrinfo *ai)
{
    return freeaddrinfo(ai);
}


/**
 * @brief 对 getaddrinfo 进行封装, 获取 ip 以及 port 的 result 链表
*/
struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype)
{
    int n;
    struct addrinfo hints, *res;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_CANONNAME; /* always return canonical name */
    hints.ai_family = family;  /* AF_UNSPEC, AF_INET, AF_INET6, etc. */
    hints.ai_socktype = socktype; /* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

    if ( (n = Getaddrinfo(host, serv, &hints, &res)) != 0 ) {
        return (NULL);
    }

    return (res); // 返回一个链表
}

/**
 * @brief  通过传入的套接字地址结构获取主机名和服务名,和 getaddrinfo 是一个互补的函数
 *         
 * @param[in]  sockaddr
 * @param[in]  addrlen
 * @param[out] host
 * @param[in]  hostlen
 * @param[out] serv
 * @param[in]  servlen
 * @param[in]  flags NI_DGRAM, NI_NAMERREQD, NI_NOFQDN, NI_NUMERICHOST...
 *                   有三个参数不会进行 DNS 查找, NI_NUMERICHOST, NI_NUMBERICSERV, NI_NUMBERICSCOPE
 *                   flags 可以进行或运算: NI_DGRAM|NI_NUMERICHOST
 *                   NI_NUMERICHOST 以字符串形式返回地址,和 inet_ntop() 等效, 内部可能通过 inet_ntop 实现
*/
int Getnameinfo(const struct sockaddr *sockaddr, socklen_t addrlen, char *host,
                socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
    int n;

    if ( (n = getnameinfo(sockaddr, addrlen, host, hostlen, serv, servlen, flags)) < 0)
        err_sys("getnameinfo error %s", hstrerror(h_errno));

    return n;
}

// 通过调用它,将当前进程变成守护进程
int Ddaemon(int nochdir, int noclose)
{
    int n;

    if ( (n = daemon(nochdir, noclose)) != 0 );
        err_sys("daemon error %s", strerror(errno));

    return n;
}

/**
 * @brief 创建两个相互连接的套接字,仅适用于 unix 域套接字
 * 
 * @param[in]  family 必须是 AF_LOCAL
 * @param[in]  type SOCK_STREAM, 流管道,效果与 pipe() 类似,区别在于 unix 域套接字是全双工的
 *                  STREAM_DGRAM 都可
 * @param[in]  protocol 必须是 0
 * @param[out] sockfd 保存套接字描述符
 * 
*/
int Socketpair(int family, int type, int protocol, int sockfd[2])
{
    int n;

    if ( (n = socketpair(family, type, protocol, sockfd)) != 0 )
        err_sys("socketpair error %s", strerror(errno));

    return n;
}