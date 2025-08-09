/**
 * @file unix network programing interface
*/
#include "util.h"

#include <arpa/inet.h>

#include <sys/types.h>

/* This operating system-specific header file defines the SOCK_*, PF_*,
   AF_*, MSG_*, SOL_*, and SO_* constants, and the `struct sockaddr',
   `struct msghdr', and `struct linger' types.  */
// #include <bits/socket.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

/*
 *	POSIX Standard: 3.2.1 Wait for Process Termination	<sys/wait.h>
 */
#include <sys/wait.h>

/* Get system-specific definitions.  */
// #include <bits/in.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <netinet/tcp.h> // for TCP_xxx defines

#include <netinet/udp.h>
/**
 * SCTP kernel Implementation: User API extensions.
 */
#include <netinet/sctp.h>

#include <netdb.h>

/***************************************************************
 * Macro
 ***************************************************************/
#define SA              (struct sockaddr)
/***************************************************************
 * Prototype
 ***************************************************************/
typedef void SigFunc(int); // 定义信号处理程序原型
/***************************************************************
 * Network
 ***************************************************************/
int Socket(int family, int type, int protocol);
int Listen(int sockfd, int backlog);
int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int Connect(int sockfd, const struct scokaddr *addr, socklen_t addrlen);
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);


extern int inet_aton(const char *cp, struct in_addr *inp); // 弃用
extern in_addr_t inet_addr(const char *cp); // 弃用, 广播地址无法转换
extern char *inet_ntoa(struct in_addr in); // 弃用, 非线程安全

const char *Inet_ntop(int af, const void *src, char *dst, socklen_t size);
int Inet_pton(int af, const char *src, void *dst);
int Shutdown(int sockfd, int howto);

int sockfd_to_family(int sockfd);

/***************************************************************
 * Socket Configuration
 ***************************************************************/
void print_socket_config(void);
bool get_fl(int fd, int flags);
void set_fl(int fd, int flags);
void clear_fl(int fd, int flags);
/***************************************************************
 * Thread Operation
 ***************************************************************/
void Pthread_mutex_lock(pthread_mutex_t *mptr);
pid_t Fork(void);
SigFunc *Signal(int signo, SigFunc *func);
int Select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);
int Pselect(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, const struct timespec *timeout,
                   const sigset_t *sigmask);
int Poll(struct pollfd *fdarray, unsigned long nfds, int timeout);

int execl(const char *path, const char *arg, ...
                /* (char  *) NULL */);
int execlp(const char *file, const char *arg, ...
                /* (char  *) NULL */);
int execle(const char *path, const char *arg, ...
                /*, (char *) NULL, char * const envp[] */);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[],
                char *const envp[]);

/***************************************************************
 * File Operation
 ***************************************************************/
ssize_t Read(int fd, void *buf, size_t count);
int Write(int fd, const void *buf, size_t count);
int Close(int fd);

ssize_t Readn(int fd, void *vptr, size_t n);
ssize_t Writen(int fd, const void *vptr, size_t n);
ssize_t readline_v1(int fd, void *vptr, size_t maxlen);
ssize_t readline_v2(int fd, void *vptr, size_t maxlen);
ssize_t readline_v3(int fd, void *vptr, size_t maxlen);

char * Fgets(char *s, int size, FILE *stream);
int Fputs(const char *s, FILE *stream);
/***************************************************************
 * 自己封装的一组网络编程 api, 兼容 ipv4 & ipv6
 ***************************************************************/
char *sock_ntop(const struct sockaddr *sa, socklen_t addrlen);
int sock_bind_wild(int sockfd, int family);
int sock_cmp_addr(const struct sockaddr *sockaddr1, 
                    const struct sockaddr *sockaddr2, socklen_t addrlen);
int sock_cmp_port(const struct sockaddr *sockaddr1, 
                    const struct sockaddr *sockaddr2, socklen_t addrlen);
int sock_get_port(const struct sockaddr *sockaddr, socklen_t addrlen);
char *sock_ntop_host(const struct sockaddr *sockaddr, socklen_t addrlen, void *ptr);
void sock_set_addr(const struct sockaddr *sockaddr, socklen_t addrlen, void *ptr);
void sock_set_port(const struct sockaddr *sockaddr, socklen_t addrlen, int port);
void sock_set_wild(const struct sockaddr *sockaddr, socklen_t addrlen);

// UDP
ssize_t Recv(int sockfd, void *buf, size_t len, int flags);
ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags,
                struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t Recvmsg(int sockfd, struct msghdr *msg, int flags);
ssize_t Send(int sockfd, const void *buf, size_t len, int flags);
ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags,
                const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t Sendmsg(int sockfd, const struct msghdr *msg, int flags);

// 获取 socket 对应的 Ip 地址和端口号
int Getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int Getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int Getsockopt(int sockfd, int level, int optname,
                void *optval, socklen_t *optlen);
int Setsockopt(int sockfd, int level, int optname,
                const void *optval, socklen_t optlen);

//SCTP
// #include <netinet/sctp.h>
int sctp_bindx(int sd, struct sockaddr *addrs, int addrcnt, int flags);
int sctp_connectx(int sd, struct sockaddr *addrs, int addrcnt,
		  sctp_assoc_t *id);
int sctp_getpaddrs(int sd, sctp_assoc_t id, struct sockaddr **addrs);
int sctp_freepaddrs(struct sockaddr *addrs);
int sctp_getladdrs(int sd, sctp_assoc_t id, struct sockaddr **addrs);
int Sctp_sendmsg(int sd, const void * msg, size_t len,
                        struct sockaddr *to, socklen_t tolen,
                        uint32_t ppid, uint32_t flags,
                        uint16_t stream_no, uint32_t timetolive,
                        uint32_t context);
int Sctp_recvmsg(int sd, void * msg, size_t len,
                        struct sockaddr * from, socklen_t * fromlen,
                        struct sctp_sndrcvinfo * sinfo, int * msg_flags);
int sctp_opt_info(int sd, sctp_assoc_t id, int opt, void *arg, socklen_t *size);
int sctp_peeloff(int sd, sctp_assoc_t assoc_id);
// SCTP 中的 shutdown():允许一个以连接套接字调用 shutdwon 关联终止后不调用 close() 然后直接连接其他 sctp 关联
// 若旧的 sctp 关联没有终止,则新关联建立失败
int Shutdown(int sockfd, int howto);
// 自己实现的一些 sctp 有关的接口
int sctp_get_no_strms(int sd, const struct sockaddr *addr, socklen_t len);
void sctp_print_sctp_info(const void *info);
void sctp_print_notification(const void *notif);

/* Domain Name System */
struct hostent *Gethostbyname(const char *hostname);
// struct hostent *gethostbyaddr(const char *addr, socklen_t len, int family);
// extern struct hostent *gethostbyaddr (const void *__addr, __socklen_t __len,
// 				      int __type);
struct servent *Getservbyname (const char *servname, const char *protoname);
struct servent *Getservbyport (int port, const char *protoname);
void print_service_info(struct servent *sptr);
void print_host_info(struct hostent *hptr);
void print_network_info(struct netent *nptr);
void print_protocol_info(struct protoent *pptr);
int Getaddrinfo (const char * hostname,
			const char * service,
			const struct addrinfo * hints,
			struct addrinfo ** result);
int Getnameinfo(const struct sockaddr *sockaddr, socklen_t addrlen, char *host,
                socklen_t hostlen, char *serv, socklen_t servlen, int flags);
const char *Gai_strerror(int error);
void FreeAddrinfo(struct addrinfo *ai);
struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype);
// 线程安全的函数(不同平台实现的这几个线程安全的函数可能有点区别,以下为 Linux 实现)
extern int gethostbyaddr_r (const void *__restrict __addr, __socklen_t __len,
			    int __type,
			    struct hostent *__restrict __result_buf,
			    char *__restrict __buf, size_t __buflen,
			    struct hostent **__restrict __result,
			    int *__restrict __h_errnop);

extern int gethostbyname_r (const char *__restrict __name,
			    struct hostent *__restrict __result_buf,
			    char *__restrict __buf, size_t __buflen,
			    struct hostent **__restrict __result,
			    int *__restrict __h_errnop);

extern int gethostbyname2_r (const char *__restrict __name, int __af,
			     struct hostent *__restrict __result_buf,
			     char *__restrict __buf, size_t __buflen,
			     struct hostent **__restrict __result,
			     int *__restrict __h_errnop);
// int Tcp_listen(const char *hostname, const char *service, socklen_t *addrlenp);

int Ddaemon(int nochdir, int noclose);

int Socketpair(int family, int type, int protocol, int sockfd[2]);
