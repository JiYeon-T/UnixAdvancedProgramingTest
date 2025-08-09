#include "test.h"
#include <stdio.h>
#include <stdlib.h>


/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h> // 移植有关的编译选项都放在这个头文件中, 如 PATH_MAX, NAME_MAX...

/*
 *	POSIX Standard: 6.5 File Control Operations	<fcntl.h>
 */
#include <fcntl.h>

/*
 *	ISO C99 Standard: 7.5 Errors	<errno.h>
 */
#include <errno.h>

/*
 *	POSIX Standard: 5.6 File Characteristics	<sys/stat.h>
 */
#include <sys/stat.h>

#include <sys/ioctl.h>

/*
 *	POSIX Standard: 7.1-2 General Terminal Interface	<termios.h>
 */
#include <termios.h> // 终端 IO 的函数
// #include <sys/disklabel.h>  // 盘标号
// #include <sys/filio.h> // 文件 IO

#include "../inc/apue.h"

// 文件描述符:使用系统调用 open() 后内核分配
// 三个标准的:STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO
// int open(const char *path, int oflag, ... /* mode_t mode */);
// int openat(int fd, const char *path, int oflag, .../* mode_t mode */);
// int create(const char *path, mode_t mode);
// int close(int fd);
// ssize_t read(int fd, void *buf, size_t nbytes);
// ssize_t write(int fd, const void *buf, size_t nbytes);
// off_t lseek(int fd, off_t offset, int whence);
// 原子操作, 内部先 lseek() 然后 read(), 但是不允许打断, 所以是原子操作
// ssize_t pread(int fd, void *buf, size_t nbytes, off_t offset);
// ssize_t pwrite(int fd, void *buf, size_t nbytes, off_t offset);
// 复制现有的文件描述符
// int dup(int fd);
// int dup2(int fd, int fd2);
// 为了保证磁盘上实际文件系统与缓冲区数据一致:
// void sync(void); // 仅仅将块缓冲区的数据加入要写的队列, 不写磁盘; 由 update 进程定期执行
// int fsync(int fd); // 更新指定文件的缓冲区到磁盘,包括数据, 文件属性等都更新
// int fdatasync(int fd); // 仅仅更新指定文件的数据部分
// 改变已经打开文件的属性
// int fcntl(int fd, int cmd, .../* int arg */); // file control
// int ioctl(int fd, int request, ...);


void file_test()
{
    long int val;
    const char *path = "./test_file.txt";
    mode_t mode;
    int fd;
    int ret;

    // get system configuration, truncate?
    val = sysconf(_PC_NO_TRUNC);
    if(val == -1) {
        err_sys("get _PC_NO_TRUNC failed");
        err_sys("errno:%d", errno);
    }
    printf("_PC_NO_TRUNC:%ld\r\n", val);

    //
    // open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    // open(path, O_CREAT | O_RDWR | O_TRUNC, mode); // create a new file

    // get current file cursor offset
    // lseek(fd, 0, SEEK_CUR);

    // lseek() 还可以用来判断涉及的文件是否可以设置偏移, 如果打开的是管道, FIFO 或套接字, 则 lseek() 返回 -1
    // 判断对于标准输入流, 是否可以 lseek()
    if(lseek(STDIN_FILENO, SEEK_CUR, 0) == -1) {
        err_sys("file fd:%d cannot sekk", STDIN_FILENO);
    } else {
        printf("seek OK\r\n");
    }

    exit(0);
}

/**
 * @fun: 制作一个有空洞的文件
*/
void hollow_test()
{
    static char buf1[] = "abcdefg";
    static char buf2[] = "xyz";
    char *path = "./file.hole";

    int fd = open(path, O_TRUNC | O_CREAT | O_RDWR);
    if(fd < 0) {
        err_sys("file %s open failed", path);
        return;
    }

    if(write(fd, buf1, strlen(buf1)) != strlen(buf1)) {
        err_sys("file %s write failed", path);
        return;
    }

    if(lseek(fd, 100, SEEK_SET) == -1) {
        err_sys("file %s lseek failed", path);
        return;    
    }

    if(write(fd, buf2, strlen(buf2)) != strlen(buf2)) {
        err_sys("file %s write failed", path);
        return;
    }

    // close(fd);

    exit(0); // process end, automatically close opened file
}

/**
 * @fun: 仅用 read 和 write 复制一个文件
*/
void copy_file_test()
{
    int n;
    char buf[BUFFERSIZE];

    while((n = read(STDIN_FILENO, buf, BUFFERSIZE)) > 0) {
        if(write(STDOUT_FILENO, buf, n) != n) {
            err_sys("write failed");
        }
    }

    if(n < 0) {
        err_quit("read error");
    }
    exit(0);
}

/**
 * @fun:
*/
void file_exist_test()
{
    int fd;
    const char *path = "./test_file.txt";
    mode_t mode = 0;

    fd = open(path, O_CREAT | O_EXCL, mode);
    if(fd < 0) {
        err_quit("file %s open failed", path);
    } else {
        printf("file %s open success fd:%d\r\n", path, fd);
    }
    
}

/**
 * @fun:
*/
void dup_file_test()
{
    int fd, fd2;
    const char *path = "./test_file.txt";
    mode_t mode = S_IRUSR | S_IWUSR;
    char buf[1024];
    int n;

    fd = open(path, O_CREAT | O_EXCL | O_RDWR | O_TRUNC, mode);
    if(fd < 0) {
        err_quit("open %s failed", path);
    } else {
        printf("file %s open success, fd:%d\r\n", path, fd);
    }

    fd2 = dup(fd);
    if(fd2 < 0) {
        err_quit("dup failed fd:%d", fd);
    } else {
        printf("dup fd:%d -> fd:%d\r\n", fd, fd2);
    }

    // 键盘输入存入 fd 所指缓存中
    while((n = read(STDIN_FILENO, buf, 1000)) > 0) {
        if(write(fd, buf, n) != n) {
            err_quit("write fd %d failed", fd);
        } else {
            printf("write fd %d success\r\n", fd);
        }

        if(write(fd2, buf, n) != n) {
            err_quit("write fd2 %d failed", fd2);
        } else {
            printf("write fd2 %d success\r\n", fd2);
        }
    }

    //

    return;
}

/**
 * @fun:
*/
void dup2_file_test()
{
    int oldfd;
    int fd;
    const char *path = "./test_file.txt";
    char buf[1024];
    int n;

    if((oldfd = open(path, O_CREAT | O_RDWR, 0644)) == -1) {
        err_quit("file %s open failed", path);
    } else {
        printf("file %s open success, oldfd:%d\r\n", path, oldfd);
    }

    // fd 使用了 stdout 的文件描述符
    // 向 oldfd 和 fd(stdout) 输出都会输出到文件 test_file.txt
    fd = dup2(oldfd, fileno(stdout));
    if(fd == -1) {
        err_quit("dup2 failed, oldfd:%d", oldfd);
    }

    while((n = read(STDIN_FILENO, buf, 5)) > 0) {
        if(write(STDOUT_FILENO, buf, n) != n) {
            err_quit("write fd:%d failed", STDOUT_FILENO);
        } else {
            // printf() 输出到标准输出的, 也会写入到文件中
            printf("fd:%d write success\r\n", STDOUT_FILENO);
        }
    }

    close(fd);

    exit(0);
}

/**
 * @fun:fcntl(int fd, int cmd, ...)
*/
void file_control_test(int argc, char **argv)
{
    int val;

    if(argc != 2) {
        err_quit("usage: a.out <descriptor#>");
    }

    // F_GETFL: 返回文件状态标志
    if((val = fcntl(atoi(argv[1]), F_GETFL, 0)) < 0) {
        err_quit("fcntl error for fd:%d", atoi(argv[1]));
    }

    printf("val:%d\r\n", val);
    // 为了兼容以前版本, 这三个文件标志位需要 & O_ACCMODE 处理
    // 0, 1, 2
    switch(val & O_ACCMODE) {
        case O_RDONLY:
            printf("read only\r\n");
            break;
        case O_WRONLY:
            printf("write only\r\n");
            break;
        case O_RDWR:
            printf("read write\r\n");
            break;
        default:
            err_sys("error val:%d", val);
            break;
    }

    // 其他值都不需要 & 处理, 直接位或即可
    if(val & O_APPEND) {
        printf("append\r\n");
    }
    if(val & O_NONBLOCK) {
        printf("no blocking\r\n");
    }
    if(val & O_SYNC) {
        printf("synchronize write\r\n");
    }
#if !defined(_POSIX_C_SOURCE) && defined(O_FSYNC) && (O_FSYNC != O_SYNC)
    if(val & O_FSYNC) {
        printf("synchronize write\r\n");
    }
#endif

    putchar('\n');

    exit(0);
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
 * @fun: 其他位不变, 仅将当前为清零
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

/**
 * @fun:
*/
void append_test()
{
    int fd;
    const char *path = "./test_file.txt";
    char *text = "text to write.";
    char buf[1024];
    int n;

    fd = open(path, O_APPEND | O_RDWR);
    if(fd < 0) {
        err_quit("file %s open failed", path);
    } else {
        printf("file %s open success, fd:%d\r\n", path, fd);
    }

    // 使用 O_APPEND 打开文件, 使用可以使用 lseek() 任意读取?
    // 可以
    n = lseek(fd, SEEK_SET, 0);
    if(n == -1) {
        err_quit("file fd:%d seek failed", fd);
    } else {
        printf("seek OK\r\n");
    }

    // while((n = read(fd, buf, 10)) != 0) {
    //     printf("read 10 bytes\r\n");
    //     for(int ix = 0; ix < 10; ++ix) {
    //         printf("%c", buf[ix]);
    //     }
    //     printf("\r\n");
    // }

    // write test
    if((n = write(fd, text, strlen(text))) != strlen(text)) {
        err_quit("fd %d write failed.\r\n", fd);
    } else {
        printf("fd %d write success\r\n", fd);
    }

    exit(0);
}