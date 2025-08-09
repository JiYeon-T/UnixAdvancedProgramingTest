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


#define HAVE_MSGHDR_MSG_CONTROL         1


ssize_t write_fd(int fd, void *ptr, size_t nbytes, int sendfd);


/**
 * @brief 用于生成一个可执行程序 openfile 给其他进程调用
 * 
 *      ./openfile <sockfd#> <filename> <mode>
*/
int main(int argc, char *argv[])
{
    int fd;

    if (argc != 4)
        err_quit("usage: openfile <sockfd#> <filename> <mode>");

    if ( (fd = open(argv[2], atoi(argv[3]))) < 0 ) {
        exit( errno > 0 ? errno : 255 );
    }

    // 发送进程不等文件描述符落地就关闭一串地的描述符(调用 exit() 时发生),因为内核知道该描述符在飞行中,
    // 从而为接收进程保存其打开状态
    // 觉体做法: 发送时 描述符引用次数+1, 因此进程退出时内核判断引用次数不为0, 不真正关闭文件 
    LOG_I("process %d send fd:%d path:%s", getpid(), fd, argv[2]);
    if ((write_fd(atoi(argv[1]), "", 1, fd)) < 0)
        exit( errno > 0 ? errno : 255);

    exit(0);
}

/**
 * @brief 通过 unix domain socket 发送文件描述符
 * 
*/
ssize_t write_fd(int fd, void *ptr, size_t nbytes, int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    // 辅助数据中携带文件描述符
#ifdef HAVE_MSGHDR_MSG_CONTROL
    union {
        struct cmsghdr cm;
        // 通过辅助数据部分发送的数据内容为:文件描述符, 大小:sizeof(int)
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr *cmptr;
    LOG_D("CMSG_SPACE:%d", CMSG_SPACE(sizeof(int)));
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);
    LOG_I("write msg_controllen:%d", msg.msg_controllen);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    LOG_I("CMSG_LEN:%d", CMSG_LEN(sizeof(int)));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int*)CMSG_DATA(cmptr)) = sendfd;
#else
    msg.msg_accrights = (caddr_t)&sendfd;
    msg.msg_accrightslen = sizeof(int);
#endif
    LOG_D("child[%d] send file fd:%d", getpid(), sendfd);

    return (Sendmsg(fd, &msg, 0));
}