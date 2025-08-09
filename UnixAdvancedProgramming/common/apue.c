#include "apue.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h> // struct cmsghdr
#include <sys/un.h>
#include <sys/resource.h>

#define CLIENT_CONN_MAX_NUM 10
#define STALE               30    // client's name can't be older than this
#define CLI_PATH            "./client_sock_"  // "/var/tmp/" 客户端套接字地址
//TODO:
// #define CLI_PATH            "./"
#define CLI_PERM            S_IRWXU     // rwx for user only

// 传送文件描述符
#define CONTROLLEN          CMSG_LEN(sizeof(int))

// 传送证书(Linux:pid, uid, gid)
#if defined(SCM_CREDS) // BSD interface
#define CREDSTRUCT          cmsgcred
#define SCM_CREDTYPE        SCM_CREDS
#define CR_UID              cmcred_uid
#elif defined(SCM_CREDENTIALS) // Linux Interface
#define CREDSTRUCT          ucred
#define CR_UID              uid
#define CREDOPT             SO_PASSCRED
#define SCM_CREDTYPE        SCM_CREDENTIALS
#else
// #error "passing credentials is not supported"
#endif

// #define RIGHTSLEN           CMSG_LEN(sizeof(int))
// #define CREDSLEN            CMSG_LEN(sizeof(struct CREDSTRUCT))
// #define CONTROLLEN          (RIGHTSLEN + CREDSLEN)



// void *path_alloc()
// {
    
// }

static struct cmsghdr *cmsghdr_p = NULL; // malloced first time


/**
 * @brief dump stat info
 * 
*/
void dump_stat(struct stat *p_stat)
{
    //TODO:
    //每一种情况的具体语义打印, 待完善
    printf("============= dump start =============\n");
    printf("st_dev:%ld\n", p_stat->st_dev);
    printf("st_uid:%ld gid:%ld\n", p_stat->st_uid, p_stat->st_gid);
    printf("st_rdev:%ld\n", p_stat->st_rdev);
    printf("st_size:%ld\n", p_stat->st_size);
    printf("st_blksize:%ld\n", p_stat->st_blksize);
    printf("st_atim:%ld st_mtim:%ld st_ctim:%ld\n", p_stat->st_atim, p_stat->st_mtim, p_stat->st_ctim);
    printf("============= dump end =============\n");
}

// 根据 <sys/wait.h> 中的 API 打印进程退出原因
void pr_exit(int status)
{
    if (WIFEXITED(status))
        printf("normal termination, exit status:%d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status)) {
        printf("abnormal termination, signal number:%d %s\n", WTERMSIG(status),
#ifdef WCOREDUMP
            WCOREDUMP(status) ? "(core file generated)" : "");
#else
            "");
#endif
    } else if (WIFSTOPPED(status)) {
        printf("child stoped, signal number:%d\n", WSTOPSIG(status));
    }
#ifdef WIFCONTINUED
    else if (WIFCONTINUED(status)) { // 
        printf("continued?\n");
    }
#endif

    return;
}

// 打印信号集中的信号类型, 获取当前进程屏蔽的信号集合
// 可以在信号的回调函数中调用
void pr_mask(const char *str)
{
    sigset_t sigset;
    int errno_save;

    errno_save = errno;
    
    if (sigprocmask(0, NULL, &sigset) < 0) { // 第二个参数为空, 则第一个参数无意义, 调用仅仅是 get()
        err_ret("sigprocmask error");
    }
    printf("%s masked signals: ", str);
    if (sigismember(&sigset, SIGINT)) {
        printf("SIGINT\t");
    }
    if (sigismember(&sigset, SIGFPE)) {
        printf("SIGFPE\t");
    }
    if (sigismember(&sigset, SIGQUIT)) {
        printf("SIGQUIT\t");
    }
    if (sigismember(&sigset, SIGUSR1)) {
        printf("SIGUSR1\t");
    }
    if (sigismember(&sigset, SIGUSR2)) {
        printf("SIGUSR2\t");
    }
    if (sigismember(&sigset, SIGALRM)) {
        printf("SIGALRM\t");
    }
    if (sigismember(&sigset, SIGFPE)) {
        printf("SIGFPE\t");
    }
    // 其他信号
    printf("\n");
    errno = errno_save;

    return;
}

/**
 * @fun: 为了避免每次都创建 flock 结构体, 增加一个接口, 直接对记录锁(文件的字节范围锁)加锁/解锁 接口
 * @param fd 文件描述符
 * @param cmd 锁的操作:F_GETLK, F_SETLK, F_SETLKW
 * @param type: 锁的类型:F_RFLCK, F_WRLCK, F_UNLCK
 * @param offset: 起始位置偏移量
 * @param whence: 起始位置
 * @param len: 要加锁区域长度
*/
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;

    return (fcntl(fd, cmd, &lock));
}

pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;

    if (fcntl(fd, F_GETLK, &lock) < 0)
        err_sys("fcntl error");
    
    if (lock.l_type == F_UNLCK) // 该位置没有被其他进程锁住
        return 0;
    
    return (lock.l_pid);
}

// 使用文件记录锁锁住一个文件
// #define lockfile(fd)    write_lock((fd), 0, SEEK_SET, 0)
int lock_file(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return (fcntl(fd, F_SETLK, &fl));
}

/**
 * @fun 读 n 个字节, 知道出错/读到 n 个
 * @param[in] fd
 * @param[in] ptr
 * @param[in] n
 * @ret
*/
ssize_t readn(int fd, void *ptr, size_t n)
{
    size_t nleft;
    ssize_t nread;

    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (nleft == n) // error
                return (-1);
            else
                break;
        } else if (nread == 0) {
            break; // EOF
        }
        nleft -= nread;
        ptr += nread;
    }

    return (n - nleft);
}

/**
 * @brief 写 n 个字节, 直到出错/写到 n 个
 * @param[in] fd
 * @param[in] ptr
 * @param[in] n
 * @ret
*/
ssize_t writen(int fd, void *ptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;

    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) < 0) {
            if (nleft == n) // error
                return (-1);
            else
                break;
        } else if (nwritten == 0) { // write failed
            break;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }

    return (n - nleft);
}

/**
 * @brief 监听客户端的连接请求, 当一个客户端想要连接时, 会通过这个名字发起连接
 * 
 * @param[in] name 一个众所周知的文件路径名, 客户端可以通过这个路径请求连接
 * @ret socket_fd 用于接收客户端连接请求的 UNIX 域套接字
*/
int serv_listen(const char *name)
{
    int fd, addr_len, err, rval;
    struct sockaddr_un un; // unix 域套接字地址

    if (strlen(name) >= sizeof(un.sun_path)) {
        errno = ENAMETOOLONG;
        return (-1);
    }

    // create a unix domain stream socket
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return (-2);
    unlink(name); // in case it already exist, 已经存在会导致 bind 失败

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

    // bind the name to the descriptor
    if (bind(fd, (struct sockaddr *)&un, addr_len) < 0) {
        rval = -3;
        goto errout;
    }

    if (listen(fd, CLIENT_CONN_MAX_NUM) < 0) { // tell kernel we're a server,can be connected by clients
        rval = -4;
        goto errout;
    }

    //服务器端的套接字绑定的地址
    //不可以删除，客户端连接的时候还需要用到这个地址
    // unlink(name);

    return (fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return (rval);
}

/**
 * @brief 接受客户端的连接
 *        该接口会阻塞等待客户进程连接
 * 
 * @param[in] listenfd serv_listen 返回的 socket_fd
 * @param[out] uidptr we also obtain the user ID from the pathname that it must bind before calling us
 * @ret return new client fd for IO if all OK
*/
int serv_accept(int listenfd, uid_t *uidptr)
{
    int clifd, err, rval;
    socklen_t len;
    time_t staletime;
    struct sockaddr_un un;
    struct stat statbuf;
    char *p_addr_path;

    if ((p_addr_path = malloc(sizeof(un.sun_path) + 1)) == NULL)
        return (-1);
    len = sizeof(un);

    printf("accept block:\n");
    //accept 可以拿到 client 的 sock_addr 信息
    //进而通过 sun_path 获取 client 信息
    // 这个 len 包含的是地址结构体的总长度
    // accept 通过第二个参数返回客户进程赋给其套接字地址的路径名, 包含客户进程 ID 的名字
    if ((clifd = accept(listenfd, (struct sockaddr *)&un, &len)) < 0) {
        free(p_addr_path);
        return (-2);
    }


    // obatin the client's uid from its calling address
    len -= offsetof(struct sockaddr_un, sun_path); // 获取 pathname 的长度
    memcpy(p_addr_path, un.sun_path, len);
    p_addr_path[len] = 0; // '\0'
    printf("client addr path:%s\n", p_addr_path);
    if (stat(p_addr_path, &statbuf) < 0) {
        rval = -3;
        goto errout;
    }

    dump_stat(&statbuf);
    printf("connect client uid:%ld gid:%d\n", statbuf.st_uid, statbuf.st_gid);

#if 0 // 验证客户进程的身份是否有效,权限检查的操作,测试可省略

#ifdef S_ISSOCK // not defined fro SVR4
    if (S_ISSOCK(statbuf.st_mode) == 0) {
        rval = -4;
        goto errout;
    }
#endif

    // 判断当前连接的用户是否有读写权限
    // TODO: 
    //用户组的成员这是要判断什么??
    //用户判断是否只有读写权限
    if ((statbuf.st_mode & (S_IRWXG | S_IRWXO)) ||
        ((statbuf.st_mode & S_IRWXU) != S_IRWXU)) {
            rval = -5;
            goto errout;
        }
    
    //验证与套接字相关联的三个时间参数不比当前时间早 30s
    staletime = time(NULL) - STALE;
    if (statbuf.st_atime < staletime ||
        statbuf.st_ctime < staletime ||
        statbuf.st_mtime < staletime) {
            rval = -6;
            goto errout;
        }
#endif

    // 返回客户端的 UID
    if (uidptr != NULL) {
        *uidptr = statbuf.st_uid; // return uid of caller
    }

    // socket 连接后, path 目录下的这个文件也就没用了
    // 这个是客户端的套接字绑定的地址
    //客户端可以不绑定地址,由操作系统分配一个
    unlink(p_addr_path); // we're done with pathname now
    free(p_addr_path);

    return (clifd);

errout:
    err = errno;
    close(clifd);
    free(p_addr_path);
    errno = err;
    return (rval);
}

/**
 * @brief create a clinent end point and connect to a server.
 *        对连接到服务器的进程进行初始化
 *
 * @param[in] server_sock_path
 * @ret returns fd if all OK, <0 return error
*/
int cli_conn(const char *server_sock_path)
{
    int fd, len, err, rval;
    struct sockaddr_un un, server_un; // unix domain socket
    int do_unlink = 0;

    if (strlen(server_sock_path) >= sizeof(un.sun_path)) {
        errno = ENAMETOOLONG;
        return (-1);
    }

    // create a unix domain stream socket
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return (-1);
    
    /* fill socket address structure with our structure */
    //这里没让系统选择默认地址, 
    //原因是:如果使用默认地址,服务器进程将不能区分各个客户端进程.
    //(如果不为 unix域套接字显式的绑定名字, 
    //内核会代表我们隐式的绑定一个地址且不会在文件系统中创建文件来表示这个套接字).
    //于是, 我们绑定自己的地址,但在开发使用套接字的客户端程序时, 通常不会这样做.
    //TODO:
    //(难道是用于调试使用???)
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "%s%05ld.test", CLI_PATH, (long)getpid());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    unlink(un.sun_path); // 防止该路径已经存在
    printf("client socket path:%s\n", un.sun_path);

    /*这在文件系统中创建了一个套接字文件,所用的名字与被绑定的路径名一样*/
    if (bind(fd, (struct sockaddr*)&un, len) < 0) {
        rval = -2;
        goto errout;
    }

    //关闭用户读写可执行以外的其他权限
    //这个套接字文件的权限也可以被修改
#if 0
    if (chmod(un.sun_path, CLI_PERM) < 0) {
        rval = -3;
        do_unlink = 1;
        goto errout;
    }
#endif
    // fill socket address structre with server's address
    memset(&server_un, 0, sizeof(server_un));
    server_un.sun_family = AF_UNIX;
    strcpy(server_un.sun_path, server_sock_path);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(server_sock_path);
    printf("client[%ld] start connect\n", getpid());
    if (connect(fd, (struct sockaddr *)&server_un, len) < 0) {
        rval = -4;
        do_unlink = 1;
        goto errout;
    }

    return (fd);
errout:
    err = errno;
    close(fd);
    if (do_unlink)
        unlink(un.sun_path);
    errno = err;
    return (rval);
}

/**
 * @brief 通过 UDS 向同一主机的其他进程发送文件描述符发生异常时,通过这个接口发送错误
 *      协议:errmsg + '\0' + status
 * 
 * @param[in] fd 通过该文件描述符发送
 * @param[in] errcode
 * @param[in] msg
 * @ret
*/
int send_err(int fd, int errcode, const char *msg)
{
    int n;

    if ((n = strlen(msg)) > 0) {
        if (writen(fd, msg, n) != n)
            return (-1);
    }

    if (errcode >= 0) // 必须是负数
        return (-1);

    if (send_fd(fd, errcode) < 0)
        return (-1);
    
    return (0);
}

/**
 * @brief  向同一主机的其他进程发送文件描述符,一般是服务器进程调用
 * 
 * @param fd UNIX 域套接字描述符, 访问权仅能通过 UDS 传送, 其他类型的 fd 都不行
 * @param fd_to_send 要发送的描述符
*/
int send_fd(int fd, int fd_to_send)
{
    struct iovec iov[1]; // 散步读/聚集写
    struct msghdr msg;
    char buf[2]; // 2 bytes 协议数据

    iov[0].iov_base = buf;
    iov[0].iov_len = 2;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    if (fd_to_send < 0) {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        buf[1] = -fd_to_send; // 负数,返回错误信息
        if (buf[1] == 0)
            buf[1] = 1; // -256
        printf("error fd_to_send:%d\n", fd_to_send);
    } else {
        // cmsghdr 控制消息头结构
        if ((cmsghdr_p == NULL) && ((cmsghdr_p = malloc(CONTROLLEN)) == NULL))
            return (-1);
        cmsghdr_p->cmsg_level = SOL_SOCKET; // For setsockopt(2)
        cmsghdr_p->cmsg_type = SCM_RIGHTS; // SCM_RIGHTS 用于传送访问权, 访问控制权, 仅能通过 UDS 传送
        cmsghdr_p->cmsg_len = CONTROLLEN; // 设置为 cmsghdr 结构的长度加一个整形的长度(文件描述符 fd 长度), sizeof(int)

        msg.msg_control = cmsghdr_p; // 文件描述符通过 msg_control 字段发送
        // CMSG_DATA 宏用户获取该整形量的指针, 转换为 cmsghdr 中指定类型的指针, 
        //例如这里传送一个 int 类型的 fd, CMSG_DATA 就转为 int* 类型
        // CMSG_LEN 返回 nbytes 长的数据对象分配的长度
        // CMSG_FIRSTHDR 指向与 msghdr 相关联的第一个 cmsghdr 结构
        // CMSG_NXTHDR 指向与 msghdr 结构图相关联的下一个 cmsghdr 结构
        msg.msg_controllen = CONTROLLEN;
        *(int*)CMSG_DATA(cmsghdr_p) = fd_to_send; // 要发送的文件描述符
        buf[1] = 0; // '\0', status 0 表示成功
    }

    buf[0] = 0; // '\0'
    if (sendmsg(fd, &msg, 0) != 2) // 聚集写，通过 sendmsg() 实现传送文件描述符的midi
        return (-1);
    printf("send 2 bytes [%d %d]\n", buf[0], buf[1]);

    return (0);
}

/**
 * @brief 接受文件描述符,一般是客户端进程调用
 * 
 * @param fd UNIX 域套接字描述符， 访问权仅能通过 UDS 传送, 其他类型的 fd 都不行
 * @param userfunc 要执行的回调函数
*/
int recv_fd(int fd, ssize_t (*error_process_func)(int fd, const void *buff, size_t nread))
{
    int newfd, nr, status;
    char *ptr;
    char buf[MAXLINE];
    struct iovec iov[1];
    struct msghdr msg;

    status = -1;
    memset(buf, -128, sizeof(buf));
    for (;;) {
        iov[0].iov_base = buf;
        iov[0].iov_len = sizeof(buf);

        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        if ((cmsghdr_p == NULL) && ((cmsghdr_p = malloc(CONTROLLEN)) == NULL))
            return (-1);
        msg.msg_control = cmsghdr_p; // 文件描述符通过 msg_control 字段发送
        msg.msg_controllen = CONTROLLEN;
        if ((nr = recvmsg(fd, &msg, 0)) < 0) { // 散布读
            err_ret("recv msg error");
            return (-1);
        } else if (nr == 0) {
            err_ret("connection closed by server");
            return (-1);
        }
        printf("receive msg(%d) nr:%d msg_controllen:%d\n", msg.msg_iovlen, nr, msg.msg_controllen);
        printf("buff print:%d %d %d\n", buf[0], buf[1], buf[2]);
        // 判断 buffer 中是否还有其他数据可以读
        for (ptr = buf; ptr < &buf[nr]; ) { // ptr++
            printf("buff:%c %d\n", *ptr, *ptr);
            if (*ptr++ == 0) { // 00 + status
                // printf("recv:%s\n", *ptr);
                if (ptr != &buf[nr - 1]) // if (nrf != 2)
                    err_ret("message format error");
                status = *ptr & 0xFF;
                if (status == 0) {
                    if (msg.msg_controllen < CONTROLLEN)
                        err_ret("status = 0 but no fd");
                    newfd = *(int*)CMSG_DATA(cmsghdr_p); // 提取描述符
                    printf("recv fd:%d\n", newfd);
                } else {
                    newfd = -status;
                }
                nr -= 2;
            }
        }

        if (nr > 0 && ((*error_process_func)(STDERR_FILENO, buf, nr) != nr))
            return (-1);

        if (status > 0) // 接收到正确的文件描述符
            return (newfd);
    }
}

/**
 * @brief 发送文件描述符, 同时接收证书
 * 
 * @param[in]
 * 
*/
// int send_fd2(int fd, int fd_to_send)
// {
//     struct CREDSTRUCT *credp;
//     struct cmsghdr *cmp;
//     struct iovec    iov[1];
//     struct msghdr   msg;
//     char buf[2];

//     iov[0].iov_base = buf;
//     iov[0].iov_len = 2;
//     msg.msg_iov = iov;
//     msg.msg_iovlen = 1;
//     msg.msg_name = NULL;
//     msg.msg_namelen = 0;
//     msg.msg_flags = 0;
//     if (fd_to_send < 0) { // 错误, 要发送的文件描述符小于 0
//         msg.msg_control = NULL;
//         msg.msg_controllen = 0;
//         buf[1] = -fd_to_send; // 表示错误
//         if (buf[1] == 0)
//             buf[1] = 1; //
//     } else {
//         if (cmsghdr_p == NULL && (cmsghdr_p = malloc(CONTROLLEN)) == NULL)
//             return (-1);
//         msg.msg_controllen = cmsghdr_p;
//         msg.msg_controllen = CONTROLLEN;
//         cmp = cmsghdr_p;
//         cmp->cmsg_level = SOL_SOCKET;
//         cmp->cmsg_type = SCM_RIGHTS;
//         cmp->cmsg_level = RIGHTSLEN;
//         *(int*)CMSG_DATA(cmp) = fd_to_send;
//         cmp = CMSG_NXTHDR(&msg, cmp);
//         cmp->cmsg_level = SOL_SOCKET;
//         cmp->cmsg_type = SCM_CREDTYPE;
//         cmp->cmsg_len = CREDSLEN;
//         credp = (struct CREDSTRUCT *)CMSG_DATA(cmp);
// #if defined(SCM_CREDENTIALS)
//         credp->uid = geteuid();
//         credp->gid = getegid();
//         credp->pid = getpid();
// #endif
//         buf[1] = 0; // 状态位
//     }

//     buf[0] = 0; // null byte flag to recv_ufd()

//     if (sendmsg(fd, &msg, 0) != 2)
//         return (-1);
    
//     return (0);
// }

/**
 * @brief 接收文件描述符, 同时接收证书
 * 
 * @param[in]
 * 
*/
int recv_fd2(int fd, uid_t *uidptr, ssize_t (*userfun)(int, const void *, size_t))
{
    
}

/**
 * @brief 打印获取进程的资源限制
 * 
 * @param[in] name resource name string
 * @param[in] resource resource id, 在 resourceh 中定义
*/
void pr_limits(char *name, int resource)
{
    struct rlimit limit;
    unsigned long long lim;

    if (getrlimit(resource, &limit)) {
        err_ret("getrlimit() error");
    }
    printf("%-14s ", name);

    if (limit.rlim_cur == RLIM_INFINITY) {
        printf("(infinite)  ");
    } else {
        lim = limit.rlim_cur;
        printf("%10d  ", lim);
    }

    if (limit.rlim_max == RLIM_INFINITY) {
        printf("(infinite)");
    } else {
        lim = limit.rlim_max;
        printf("%10d", lim);
    }

    putchar((int)'\n');

    return;
}