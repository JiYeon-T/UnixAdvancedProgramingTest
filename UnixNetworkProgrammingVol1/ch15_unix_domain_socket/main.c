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


#define UNIX_DOMAIN_SOCKET_TEST         0
#define UNIX_DOMAIN_SOCKET_FP_TEST      0 // 文件描述符传递测试
#define UNIX_DOMAIN_SOCKET_CA_TEST      1 // 凭证传输测试

#define HAVE_MSGHDR_MSG_CONTROL         1
#define CONTROL_LEN     ((sizeof(struct cmsghdr)) + sizeof(struct cmsgcred))

int unix_domain_socket_test1(int argc, char *argv[]);



int main(int argc, char *argv[])
{
#if UNIX_DOMAIN_SOCKET_TEST
    unix_domain_socket_test1(argc, argv);
#endif

#if UNIX_DOMAIN_SOCKET_FP_TEST
    unix_domain_socket_trans_fp_test(argc, argv);
#endif

#if UNIX_DOMAIN_SOCKET_CA_TEST
    unix_domain_socket_trans_cert_test(argc, argv);
#endif
}

/**
 * @brief
 *          ./unixbind <pathname>
 * @note 参数必须是绝对路径名,而不是一个相对路径名,否则将导致该应用程序的调用依赖当前目录
*/
int unix_domain_socket_test1(int argc, char *argv[])
{
    int sockfd;
    socklen_t len;
    struct sockaddr_un addr1, addr2; /* unix 域套接字结构 */

    if (argc != 2)
        err_quit("usage: unixbind <pathname>");
    
    sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0); // 字节流

    // NOTE: 如果文件系统中已经存在该文件名, bind 将会失败,因此我们必须先删除这个路径名
    // 防止它已经存在
    unlink(argv[1]); // OK if this fails

    bzero(&addr1, sizeof(addr1));
    addr1.sun_family = AF_LOCAL;
    strncpy(addr1.sun_path, argv[1], sizeof(addr1.sun_path) - 1);
    // Bind(sockfd, (const struct sockaddr *)&addr1,  sizeof(addr1));
    Bind(sockfd, (const struct sockaddr *)&addr1,  SUN_LEN(&addr1));

    len = sizeof(addr2);
    Getsockname(sockfd, &addr2, &len);
    LOG_I("bind socket name:%s len:%d", addr2.sun_path, len);
    exit(0);
}

/**
 * @brief 描述符传递,用于没有血缘关系的进程之间传递描述符
 * 有血缘关系:父进程 -> 子进程, 进程间共享资源的方法,直接由父进程传给子进程
 * 子进程->父进程: socketpair() 创建连接的 unix 域套接字,然后 sendmsg()
 * 没有血缘关系的进程:
 * 先创建套接字 -> 连接 -> sendmsg()
 * 
 * 描述符传递步骤:
 * (1) 创建 unix 域套接字(tcp, udp 可能还会丢数据);
 * (2) 可以传递的描述符不限类型, open(), pipe(), mkfifo(), socket(), accept() 等返回的描述符都可以传递
 * (3) 将描述符放在 sendmsg() 接口的辅助数据中;
 * (4) 接收进程调用 recvmsg() 接收描述符,(不是简单的传递描述符数值,而是创建一个新的描述符,在内核中指向相同的文件表项);
*/
#if UNIX_DOMAIN_SOCKET_FP_TEST
static int my_open(const char *pathname, int mode);
ssize_t read_fd(int fd, void *ptr, size_t nbytes, int *recvfd);

/**
 * @brief ./mycat pathname
*/
int unix_domain_socket_trans_fp_test(int argc, char *argv[])
{
    int fd, n;
    char buff[BUFFERSIZE];

    if (argc != 2)
        err_quit("usage: mycat <pathname>");
    
    if ( (fd = my_open(argv[1], O_RDONLY)) < 0 )
        err_sys("cannot open %s", argv[1]);
    
    while ( (n = Read(fd, buff, sizeof(buff))) > 0 )
        Write(STDOUT_FILENO, buff, n);

    exit(0);
}

/**
 * @brief 调用可执行文件 openfile 读取文件,并通过 uds 返回 openfile "打开的文件描述符"
 *       给 openfile 传递参数可以在 execl() 时传递,也可以通过 uds 传递
 *
 * @param[in]  要打开的文件路径
 * @param[in]  打开文件模式
 * @retval 文件描述符
*/
static int my_open(const char *pathname, int mode)
{
    int fd = -1; 
    int sockfd[2], status;
    pid_t childpid;
    char c, argsockfd[10], argmode[10];

    // 创建一对已连接的 uds
    Socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);

    if ( (childpid = Fork()) == 0) { // child
        Close(sockfd[0]);
        snprintf(argsockfd, sizeof(argsockfd), "%d", sockfd[1]);
        snprintf(argmode, sizeof(argmode), "%d", mode);
        execl("openfile", "openfile",
                argsockfd, pathname, argmode, (char*)NULL);
        err_sys("execl error %s", strerror(errno));
    }

    LOG_I("parent continue running -> waitpid");
    // parent:
    // close the end we don't use and wait child terminate
    Close(sockfd[1]);
    waitpid(childpid, &status, 0);
    LOG_I("child exit");
    if (WIFEXITED(status) == 0) {
        LOG_E("child did not terminate");
    }
    if ( (status = WEXITSTATUS(status)) == 0)
        // 传递文件描述符时不可以仅传输描述符,因此这里传输 1byte 数据
        read_fd(sockfd[0], &c, sizeof(c), &fd);
    else {
        errno = status; // set errno's value from child's status
        fd = -1;
        LOG_E("child exit status:%d errno:%s", errno, strerror(errno));
    }

    Close(sockfd[0]);
    LOG_I("parent[%d] received fd:%d", getpid(), fd);
    // 父进程接收到的描述符和子进程发送的描述符数值是不一样的,
    // 但是这两个描述符其实是指向同一个文件表项的描述符

    return (fd);
}

/**
 * @brief 从 uds 中读取已打开的描述符
 * 
 * @param[in]  fd 
 * @param[in]  ptr 
 * @param[in]  nbytes 
 * @param[out] recvfd 
*/
ssize_t read_fd(int fd, void *ptr, size_t nbytes, int *recvfd)
{
    struct msghdr msg;
    struct iovec iov[1];
    ssize_t n;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    // msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
    msg.msg_iovlen = 1;

#ifdef HAVE_MSGHDR_MSG_CONTROL
    union {
        struct cmsghdr cm;
        // 通过辅助数据部分发送的数据内容为:文件描述符, 大小:sizeof(int)
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr *cmptr; // 指向辅助数据的指针

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);
    LOG_I("read msg_controllen %d", msg.msg_controllen);
#else
    int newfd;

    msg.msg_accrights = (caddr_t)&newfd;
    msg.msg_acccrightslen = sizeof(int);
#endif

    if ( n = recvmsg(fd, &msg, 0) <= 0 ) {
        LOG_E("recv msg failed:%s", strerror(errno));
        return (n);
    }
#ifdef HAVE_MSGHDR_MSG_CONTROL
    // TODO:
    // 从辅助数据中获取描述符
    if ( (cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
             cmptr->cmsg_len == CMSG_LEN(sizeof(int)) ) {
        if (cmptr->cmsg_level != SOL_SOCKET) {
            err_quit("controlevel != SOL_SOCKET");
        }
        if (cmptr->cmsg_type != SCM_RIGHTS)
            err_quit("cmsg_type != SCM_RIGHTS");
        *recvfd = *((int*)CMSG_DATA(cmptr));
        LOG_I("received fd:%d", *recvfd);
    } else {
        *recvfd = -1; /* file descriptor was not passed */
    }
#else
    if (msg.msg_accrightslen == sizeof(int))
        *recvfd = newfd;
    else
        *recvfd = -1; /* file descriptor was not passed */
#endif

    return (n);
}
#endif /* UNIX_DOMAIN_SOCKET_FP_TEST */

#if UNIX_DOMAIN_SOCKET_CA_TEST
/**
 * @brief 通过 UDS 传递用户凭证, 服务器请求客户用户凭证测试
 *        FreeBSD 平台的写法
*/
void unix_domain_socket_trans_cert_test(int argc, char *argv[])
{

}

/**
 * @brief 凭证信息总是可以通过 UDS 传递, 
 *        接收进程只需要在调用 recvmsg() 时提供一个可以存方凭证辅助数据的空间即可
 *        发送进程调用 sendmsg() 发送数据时,必须作为辅助数据包含一个 cmsgcred 结构,
 *        
 *      "NOTE:虽然 FreeBSD 要去发送数据的时候需要提供一个 cmsgcred 结构, 但是其内容却是由内核填写的
 *      ,发送进程无法伪造. 这么做使得通过 UDS 传递凭证成为服务器验证客户身份的可靠手段"
 *        
*/
ssize_t read_cred(int fd, void *ptr, size_t nbytes, struct cmsgcred *cmsgcredptr)
{
    struct msghdr msg;
    struct iovec iov[1];
    char control[CONTROL_LEN];
    int n;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);
    msg.msg_flags = 0;

    if ( (n = recvmsg(fd, &msg, 0)) < 0 ) {
        LOG_E("recvmsg failed");
        return (n);
    }

    cmsgcredptr->cmcred_ngroups = 0; // indicates no credentials returned
    if (cmsgcredptr && msg.msg_controllen > 0) {
        struct cmsghdr *cmptr = (struct cmsghdr *)control;
        if (cmptr->cmsg_len < CONTROL_LEN)
            err_quit("cmsg_len = %d", cmptr->cmsg_len);
        if (cmptr->cmsg_level != SOL_SOCKET) {
            err_quit("controlevel != SOL_SOCKET");
        }
        if (cmptr->cmsg_type != SCM_CREDS)
            err_quit("cmsg_type != SCM_CREDS");
        memcpy(cmsgcredptr, CMSG_DATA(cmptr), sizeof(struct cmsgcred));
    }

    return (n)
}

/**
 * @brief 该函数在父进程接受连接并创建子进程后调用
*/
void str_echo(int sockfd)
{
    ssize_t n;
    int i;
    char buff[MAXLINE];
    struct cmsgcred cred;

again:
    while ( (n = read_cred(sockfd, buff, MAXLINE, &cred)) > 0) {
        if (cred.cmred_ngroups == 0) {
            printf("no credentials returned\n");
        } else {
            printf("PID of sender = %d\n", cred.cmred_pid);
            printf("read user ID=%d\n", cred.cmred_uid);
            printf("real group ID =%d\n", cred.cmred_euid);
            printf("%d groups:", cred.cmred_ngoups - 1);
            for (i = 1, i < cred.cmred_ngroups; i++) {
                printf("%d\n", cred.cmred_groups[i]);
            }
            printf("\n");
        }
        Writen(sockfd, buff, n);
    }

    if ( n < 0 && errno == EINTR)
        goto again;
    else if (n < 0)
        err_sys("str_echo read error");
}
#endif /* UNIX_DOMAIN_SOCKET_CA_TEST */