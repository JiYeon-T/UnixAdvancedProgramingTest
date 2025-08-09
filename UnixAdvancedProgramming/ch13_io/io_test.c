#include "../common/apue.h"
#include <pthread.h>
#include <time.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*	POSIX 1003.1g: 6.2 Select from File Descriptor Sets <sys/select.h>  */
#include <sys/select.h>

#include <poll.h>

/*
 * ISO/IEC 9945-1:1996 6.7: Asynchronous Input and Output
 */
#include <aio.h> // -lrt

/* This file defines `struct iovec'.  */
#include <sys/uio.h>

#include <sys/mman.h>

// strace 命令可以查看当前可执行文件调用的系统调用的情况

/***************************************************************
 * Function Declaration
 ***************************************************************/
// 控制
int fcntl(int fd, int cmd, .../*struct flock flockptr*/);

// IO 多路转接
int select(int maxfdp1, fd_set * restrict readfds, fd_set * restrict writefds, fd_set * restrict exceptfds,
            struct timeval * restrict tvptr);
int pselect(int maxfdp1, fd_set * restrict readfds, fd_set * restrict writefds, fd_set * restrict exceptfds,
            const struct timespec * restrict tsptr, const sigset_t * restrict sigmask);
// int FD_ISSET(int fd, fd_set *fdset);
// void FD_CLR(int fd, fd_set *fd_set);
// void FD_SET(int fd, fd_set *fd_set);
// void FD_ZERO(fd_set *fdset);
int poll(struct pollfd fdarray[], nfds_t nfds, int timeout);

// 异步 IO
// int aio_read(struct aiocb *aiocb); //异步读请求
// int aio_write(struct aiocb *aiocb); //异步写请求
// int aio_error(const struct aiocb *aiocb); // 检查异步请求的状态
// ssize_t aio_return(const struct aiocb *aiocb); // 获取异步请求完整的返回值(异步io操作完成的结果)
// int aio_suspend(const struct aiocb *const list[], int nent, const struct timespec *timeout); // 挂起调用进程直到一个或者多个异步请求已完成, 当某一个完成异步请求完成, 也会返回一次, 
// int aio_cancel(int fd, struct aiocb *aiocb); // 取消异步请求
// int lio_listio(int mode, struct aiocb *restrict const lilst[restrict], int nent, struct sigevent * restrict sigev); // 发起一系列异步操作请求, 数组中
// int aio_fsync(int op, struct aiocb *aiocb);

// 散步读 & 聚集写
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

ssize_t readn(int fd, void *buf, size_t nbytes);
ssize_t writen(int fd, void *buf, size_t nbytes);

// 存储映射 io
// void *mmap(void *addr, size_t len, int prot, int flag, off_t off);
// int mprotect(void *addr, size_t len, int prot);
// int msync(void *addr, size_t len, int flags);
// int munmap(void *addr, size_t len);
/***************************************************************
 * Static Variables
 ***************************************************************/
static char buf[10 * KB];

static void set_fl(int fd, int flags);
static void clr_fl(int fd, int flags);

// 1. 非阻塞 IO
void io_test1(void)
{
    int ntowrite, nwrite;
    char *ptr;

    ntowrite = read(STDIN_FILENO, buf, sizeof(buf)); // 阻塞 IO
    fprintf(stderr, "read:%d bytes\n", ntowrite);

    set_fl(STDIN_FILENO, O_NONBLOCK); // 非阻塞

    ptr = buf;
    while (ntowrite > 0) {
        errno = 0;
        nwrite = write(STDOUT_FILENO, ptr, ntowrite);
        fprintf(stderr, "nwrite=%d bytes errno:%d\n", nwrite, errno);
        if (nwrite > 0) {
            ptr += nwrite;
            ntowrite -= nwrite;
        }
    }
    clr_fl(STDOUT_FILENO, O_NONBLOCK);

    return;
}

// set fd
static void set_fl(int fd, int flags)
{
    int val;

    if ((val = fcntl(fd, F_GETFD, 0)) < 0) {
        err_quit("get fcntl failed");
    }

    val |= flags;

    if ((val = fcntl(fd, F_SETFD, 0)) < 0) {
        err_quit("set fcntl failed");
    }

    return;
}

// clear fd
static void clr_fl(int fd, int flags)
{
    int val;

    if ((val = fcntl(fd, F_GETFD, 0)) < 0) {
        err_quit("get fcntl failed");
    }

    val &= ~flags;

    if ((val = fcntl(fd, F_SETFD, 0)) < 0) {
        err_quit("set fcntl failed");
    }

    return;
}

// 文件记录锁(字节范围锁)
void file_record_test1(void)
{
    char *path = "./test_file.txt";
    int fd;
    int cnt = 0;

    fd = open(path, O_RDWR);
    ASSERT(fd > 0);
    printf("file:%s lock state r:%d w:%d\n", path, is_read_lockable(fd, 0, SEEK_SET, 0), 
        is_write_lockable(fd, 0, SEEK_SET, 0));
    writew_lock(fd, 0, SEEK_SET, 0); // 对整个文件加锁

    while(1) {
        printf("running %d\n", ++cnt);
        sleep(1);
    }
    close(fd);
    un_lock(fd, 0, SEEK_SET, 0); // 杀掉进程会自动释放文件记录锁的, 这个和 mutex 好像还不太一样
}

// 返回当前正在保持锁的进程的 pid
static void lockabyte(const char *name, int fd, off_t offset);


static void lockabyte(const char *name, int fd, off_t offset)
{
    if (writew_lock(fd, offset, SEEK_SET, 1) < 0)
        err_sys("%s:writew_lock err, name", name);
    printf("%s:got the lock, byyte:%lld\n", name, (long long)offset);
}

void dead_lock_test(void)
{
    int fd;
    pid_t pid;

    // if ((fd = creat("templock", FILE_MODE)) < 0)
    if ((fd = open("templock", O_CREAT|O_WRONLY|O_TRUNC)) < 0)
        err_sys("create err");
    if (write(fd, "ab", 2) != 2)
        err_sys("write err");
    
    // TELL_WAIT(); // 父子进程间同步
    if ((pid = fork()) < 0) {
        err_sys("fork failed");
    } else if (pid == 0) { // child
        lockabyte("child", fd, 0);
        // TELL_PARENT(getppid());
        // WAIT_PARENT();
        lockabyte("child", fd, 1);
    } else { // parent
        lockabyte("parent", fd, 1);
        // TELL_CHILD(pid);
        // WAIT_CHILD();
        lockabyte("parent", fd, 0);
    }
}

// 设置强制性文件记录锁
void file_lock_test(int argc, char *argv[])
{
    int fd;
    pid_t pid;
    char buf[5];
    struct stat statbuf;

    if (argc != 2) {
        fprintf(stderr, "usage:%s file name\n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
        err_sys("open failed");
    
    if (write(fd, "abcdef", 6) != 6)
        err_ret("write err");

    // 关闭组可执行权限
    if (fstat(fd, &statbuf) < 0)
        err_ret("fstat() failed");
    
    if (fchmod(fd, (statbuf.st_mode & ~S_IXGRP) | S_ISGID) < 0)
        err_ret("fchmod failed");

    if ((pid = fork()) < 0) {
        err_ret("fork error");
    } else if (pid > 0) { // parent
        if (write_lock(fd, 0, SEEK_SET, 0) < 0)
            err_sys("write lock err");
        // TELL_CHILD(pid);
        sleep(2);
        if (waitpid(pid, NULL, 0) < 0)
            err_ret("waitpid err");
    } else { // child
        // TELL_PARENT()
        sleep(2);
        set_fl(fd, O_NONBLOCK); // 设置非阻塞

        // 尝试获取读锁
        if (read_lock(fd, 0, SEEK_SET, 0) != -1)
            err_ret("get read lock success");
        printf("read lock get failed errno:%d\n", errno);

        //尝试强制读取该文件,查看 errno 来判断系统是否支持"强制性文件记录锁"
        if (lseek(fd, 0, SEEK_SET) == -1)
            err_sys("seek failed");

        if (read(fd, buf, 2) < 0) {
            err_ret("read failed(mandatory locking works) errno:%d", errno);
        } else {
            printf("read success(mandatory locking does not work)\n");
        }
    }
    exit(0);

}

// IO 多路转接 poll(), select(), pselect()
void select_test(void)
{

}

/**
 * @brief pselect 在 select 的基础上增加了屏蔽信号的功能
 *        允许在调用期间更改信号掩码，这意味着可以阻塞某些信号，防止它们在 pselect 调用期间中断它。
 *        https://cloud.tencent.com/developer/information/linux%20pselect
*/
void pselect_test(void)
{
    fd_set readfds;
    struct timespec timeout;
    int fd; 
    sigset_t newmask, oldmask, zeromask;

    fd = open("example.txt", O_RDONLY);

    if (fd < 0) {
        perror("open\n");
        exit(EXIT_FAILURE);
    }

    timeout.tv_sec = 5;
    timeout.tv_nsec = 0;

    sigemptyset(&zeromask);
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);
    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        // TODO:
        // 测试 intr_flag 变量之前,我们阻塞 SIGINT 信号. 当 pselect 被调用时,我们先传入空集,
        // 即 zeromask 替代进程的信号掩码,然后检查描述符.
        // pselect 返回后,进程的信号掩码又被重置回 pselect 之前的值,即(阻塞 SIGINT 信号)
        int nready = pselect(fd + 1, &readfds, NULL, NULL, &timeout, &zeromask);
        if (nready < 0) {
            perror("pselect");
            break;
        } else if (nready > 0) {
            char buff[MAXLINE];
            ssize_t n = read(fd, buff, sizeof(buff));
            if ( n < 0) {
                perror("read\n");
                break;
            } else if (n == 0) {
                printf("EOF\n");
                break;
            }
            printf("read %d bytes: %s\n", n, buff);
        } else {
            printf("timeout occurred\n");
        }
    }

    close(fd);
    return 0;
}

void async_io_test(void)
{

}

#define COPYINCR        GB

void mmap_io_test(int argc, char *argv[])
{
    int fdin, fdout;
    void *src, *dst;
    size_t copysz;
    struct stat sbuf;
    off_t fsz;

    if (argc != 3)
        err_ret("usage:%s <fromfile> <tofile>", argv[0]);
    
    if ((fdin = open(argv[1], O_RDONLY)) < 0)
        err_ret("open file:%s failed", argv[1]);
    
    if ((fdout = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) < 0)
        err_ret("open file:%s failed", argv[2]);

    if (fstat(fdin, &sbuf) < 0) // GET INput file size
        err_sys("fstat failed");
    
    if (ftruncate(fdout, sbuf.st_size) < 0) // set output file size to
        err_ret("fstruncate failed");
    
    while (fsz < sbuf.st_size) {
        if ((sbuf.st_size - fsz) > COPYINCR)
            copysz = COPYINCR;
        else
            copysz = sbuf.st_size - fsz;
        
        // 创建存储映射 IO
        if ((src = mmap(0, copysz, PROT_READ, MAP_SHARED, fdin, fsz)) == MAP_FAILED)
            err_sys("mmap failed");
        if ((dst = mmap(0, copysz, PROT_READ | PROT_WRITE, MAP_SHARED, fdout, fsz)) == MAP_FAILED)
            err_sys("mmap failed");
        
        // copy
        memcpy(dst, src, copysz);
        munmap(src, copysz);
        munmap(dst, copysz);
        fsz += copysz;
        printf("mmap copy:%ld\n", copysz);
    }

    // 如果要确保正常写入, 则要在进程终止前调用 msync()

    return;
}