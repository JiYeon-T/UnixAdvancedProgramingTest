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

// #include <sys/devpoll.h> // Solaris


// 与 read/write 相比,增加了末尾的 flags 参数, 可以增加一个 io 标志
// 但是该参数为值参数,只能用于进程向内核传送数据,而不能用于内核传出数据, 
// 当需要从内核返回数据时,不可以调用 sendto/send recv/recvfrom, 只能调用 sendmsg/recvmsg
ssize_t recv(int sockfd, void *buff, size_t nbytes, int flags);
ssize_t send(int sockfd, const void *buff, size_t nbytes, int flags);
// 分散读与聚集写:
ssize_t readv(int filedes, const struct iovec *iov, int iovcnt);
ssize_t writev(int filedes, const struct iovec *iov, int iovcnt);
// 可以带辅助数据/从内核返回数据的读写操作(仅套接字描述符可用)
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
ssize_t sendmsg(int sockfd, struct msghdr *msg, int flags);

// struct cmsghdr
// 一些有用的宏
// CMSG_FIRSTHDR // 返回指向第一个 cmsghdr 结构的指针
// CMSG_NXTHDR // 返回下一个 cmsghdr 结构的指针 
// CMSG_DATA // 返回与 cmsghdr 关联的数据的第一个字节的指针
// CMSG_LEN // 返回给定数据量下存放到 CMSSG_LEN 中的值
// CMSG_SPACE // 返回给定数据量下,一个辅助数据对象总的大小

// 标准 io 函数库提供的一些接口:
FILE *fopen(const char *path, const char *mode);
FILE *fdopen(int fd, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *stream);


/* 套接字 Io 操作上设置超时的方法有 3 种:
 * 1. alarm 设置超时时间;
 * 2. select
 * 3. SO_RCVTIMEO, SO_SNDTIMEO 套接字选项; 
 *
 */
int connect_timeo(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec);
void dg_client_v1(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen);
int readable_timeo(int fd, int sec);
void dg_client_v2(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen);
void dg_cli_v3(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen);
void str_echo(int sockfd);




int main(int argc, char *argv)
{

}

static void connect_alarm_handler(int signo)
{
    return; /* Just interrupt the connect() */
}


/**
 * @brief 调用 alarm 为 connect 设置超时
 * 
 * @note 1.内核会给 connect() 设置一个默认的超时时间,该接口的超时时间不能大于这个时间
 *        例如: berkeley 设置的超时时间为 75s.
 *       2.尽管本例子非常简单,但是在多线程化程序中正确使用信号却非常困难,因此我们建议只是在未线程化或者
 *        单线程的例子中使用该接口(谨慎使用信号!!!!!!!!!!!)
*/
int connect_timeo(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec)
{
    SigFunc *sigfunc;
    int n;
    int remain_sec;

    sigfunc = Signal(SIGALRM, connect_alarm_handler); // signal 返回之前该信号注册的函数
    if ( (remain_sec = alarm(nsec)) != 0)
        err_sys("connect_time: alaram was already set %d", remain_sec);
    
    // connect 阻塞会被 alarm 信号中断,并且 errno 设置为 EINTR
    if ( (n = connect(sockfd, saptr, salen)) < 0) {
        close(sockfd);
        if (errno == EINTR)
            errno = ETIMEDOUT;
    }
    alarm(0); // turn off the alarm
    Signal(SIGALRM, sigfunc); // restore previous signal handler

    return (n);
}

static void recvfrom_alarm_handler(int signo)
{
    return; /* Just interrupt the recvfrom() */
}

/**
 * @brief 为 recvfrom 设置超时
*/
void dg_client_v1(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE];

    Signal(SIGALRM, recvfrom_alarm_handler);

    while ( Fgets(sendline, sizeof(sendline), fp) != NULL ) {
        Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

        alarm(5);
        if ( (n = Recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) < 0 ) {
            if (errno == EINTR)
                fprintf(stderr, "socket timeout");
            else
                err_sys("recvfrom error");
        } else {
            alarm(0); // close alarm
            recvline[n] = '\0';
            Fputs(recvline, stdout);
        }
    }
}

/**
 * @brief 使用 select 进行超时时间设置
 *        本函数不执行读操作,只是等待描述符变为可读后返回
*/
int readable_timeo(int fd, int sec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return (Select(fd+1, &rset, NULL, NULL, &tv));
}

/**
 * @brief 为 recvfrom 设置超时, 使用 readable_timeo 接口
*/
void dg_client_v2(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE];

    Signal(SIGALRM, recvfrom_alarm_handler);

    while ( Fgets(sendline, sizeof(sendline), fp) != NULL ) {
        Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

        if (readable_timeo(sockfd, 5) == 0) {
            fprintf(stderr, "socket timeout");
        } else {
            n = Recvfrom(sockfd, recvline, sizeof(recvline), 0, NULL, NULL);
            recvline[n] = '\0'; // NULL terminate
            Fputs(recvline, stdout);
        }
    }
}

/**
 * @brief 使用套接字选项, 但是这种阻塞超时方式仅套接字描述符可以使用,其他都不可以
 * 
*/
void dg_cli_v3(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE];
    struct timeval tv;

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    Setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while ( Fgets(sendline, sizeof(sendline), fp) != NULL ) {
        Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

        n = Recvfrom(sockfd, recvline, sizeof(recvline), 0, NULL, NULL);
        if (n < 0) {
            if (errno == EWOULDBLOCK) {
                fprintf(stderr, "socket timeout");
                continue;
            } else {
                err_sys("recvfrom error:%s", strerror(errno));
            }
        }
        recvline[n] = '\0'; // NULL terminate
        Fputs(recvline, stdout);
    }
}

/**
 * @brief recvmsg 辅助数据测试
*/
void ancillary_data_test(void)
{

}

/**
 * @brief 遍历 cmsghdr 结构的伪代码
*/
void cmsghdr_iterate_pseudo_code(void)
{
    struct msghdr msg;
    struct cmsghdr *cmsgptr;

    for (cmsgptr = CMSG_FIRSTHDR(&msg); cmsgptr != NULL; cmsgptr = CMSG_NXTHDR(&msg, cmsgptr)) {
        // 这里以 unix 域套接字传递文件描述符为例
        if (cmsgptr->cmsg_level == SOL_SOCKET && cmsgptr->cmsg_type == SCM_RIGHTS) {
            char *ptr;
            ptr = CMSG_DATA(cmsgptr);
            /* process data pointed to ptr */
        }
    }
}

/**
 * @brief 使用标准 IO 的 str_echo 函数,
 *        服务器知道我们输入 EOF 才返回客户端内容,因为默认为完全缓冲
 *        解决方法:1. setvbuf() 设置缓冲方式; 2.fflush()清空缓冲区,; 会有其他副作用
 * NOTE:最好的解决办法是在套接字上避免使用标准 IO 函数库
*/
void str_echo(int sockfd)
{
    char line[MAXLINE];
    FILE *fpin, *fpout;

    // 将 sockfd 创建两个流,一个用于输入,一个用于输出
    fpin = fdopen(sockfd, "r");
    fpout = fdopen(sockfd, "w");
    while (Fgets(line, MAXLINE, fpin) != NULL)
        Fputs(line, fpout);
}

/**
 * @brief 其他高级轮询技术_不是 POSIX 标准,不兼容所有平台
 *        Solaris - /dev/poll
 * 
 * @param[in]  fp
 * @param[in]  sockfd
*/
#if 0
void str_cli(FILE *fp, int sockfd)
{
    int stdineof;
    char buf[MAXLINE];
    int n;
    int wfd;
    struct pollfd pollfd[2];
    struct dvpoll dopoll;
    int i;
    int result;

    wfd = open("/dev/poll", O_RDWR);

    pollfd[0].fd = fileno(fp);
    pollfd[0].events = POLLIN;
    pollfd[0].revents = 0;

    pollfd[1].fd = sockfd;
    pollfd[1].events = POLLIN;
    pollfd[1].revents = 0;

    Write(wfd, pollfd, sizeof(struct pollfd) * 2);
    stdineof = 0;

    for (; ;) {
        // block until /dev/poll says something is ready
        dopoll.dp_timeout = -1;
        dopoll.dp_nfds = 2;
        dopoll.dp_fds = pollfd;
        result = ioctl(wfd, DP_POLL, &dopoll); // ioctl() 一直阻塞到监听的事件发生或者超时
        
        // loop throuth ready file descriptors
        for (i = 0; i < result; ++i) {
            if (dopoll.dp_fds[i].fd == sockfd) { // sockfd is readable
                if ( (n = Read(sockfd, buf, MAXLINE)) == 0 ) {
                    if (stdineof == 1)
                        return; // normal termination
                    else
                        err_quit("strcli server terminated prematurely");
                }
                Write(fileno(stdout), buf, n);
            } else { // input is readable
                if ( (n = Read(fileno(fp), buf, MAXLINE)) == 0 ) {
                    stdineof = 1;
                    Shutdown(sockfd, SHUT_WR); // send FIN
                    continue;
                }
                Write(sockfd, buff, n)
            }
        }
    }
}
#endif

/**
 * @brief 其他高级轮询技术_不是 POSIX 标准,不兼容所有平台
 *        FreeBSD - kqueue, 该接口允许进程向内核注册描述所关注的 kqueue 事件的事件过滤器
 *        事件除了与 select() 类似的 io 以及超时外,还包括文件被修改,删除,进程跟踪和信号处理
 * 
 * @param[in]  fp
 * @param[in]  sockfd
*/
#if 0
extern kqueue(void);
extern kevent(int kq, const struct kevent *changelist, int nchanges,
                struct kevent *eventlist, int nevents, const struct timespec *timeout);
extern EV_SET(struct kevent *kev, uintptr_t ident, short filter, u_short flags, u_int fflags,
                intptr_t data, void *u_data);

void str_cli(FILE *fp, int sockfd)
{
    // TODO:

}
#endif

// TODO: 下面这个函数测试
// str_echo test
void str_echo_tes(void)
{
    for (; ;) {
        if ( n = Recv(sockfd, recvlient, MAXLINE, MSG_PEEK) == 0) // MSG_PEEK, 不从内核清空数据
            break; // server closed
        Ioctl(sockfd, FIONREAD, &npend); // 获取缓冲区中数据数量
        printf("%d bytes from peek, %d bytes pending", n, npend);

        n = Read(sockfd, recvlient, MAXLINE);
        recvline[n] = '\0';
        Fputs(recvlient, stdout);
    }
}