/**
 * @fun 架构模型:
 * 客户端:发送路径(可以是设备/文件/套接字等等)以及读写权限
 * 服务器:返回文件描述符
 * 为什么客户端不自己直接打开呢? 反正在同一台电脑上.................
*/
#include <sys/uio.h>

#include "ch16.h"
#include  "../ch17_IPC/ch17.h"


#define BUFFSIZE        8192


/**
 * @fun 将路径名按照指定格式传送到服务器端,获取到服务器返回的 fd 后,将内容读出,打印到 stdout
*/
int start_open_client_v1(int argc, char *argv[])
{
    int n, fd;
    char buf[BUFFSIZE];
    char line[BUFFSIZE];

    printf("client start\n");
    // read file name and to cat from stdin
    while (fgets(line, BUFFSIZE, stdin) != NULL) {
        // 修改服务器为行缓冲
        if (line[strlen(line) - 1] == '\n')
            line[(strlen(line) - 1)] = 0; // \0
        
        // open file:fd receive from server
        printf("send to server(%d):%s\n", strlen(line) + 1, line);
        if ((fd = cs_open(line, O_RDONLY)) < 0)
            continue;
        
        // read data and print to stdout
        while ((n = read(fd, buf, BUFFSIZE)) > 0)
            if (write(stdout, buf, n) != n)
                err_sys("write failed");
        if (n < 0)
            err_sys("read failed");
        close(fd);
    }

    exit(0);
}

/**
 * @fun Open the file by sending the "name" and "oflag" to server
 * 创建管道以后,对服务器进程进行 fork() 以及 exec()
*/
int cs_open(char *name, int oflag)
{
    pid_t pid;
    int len;
    char buf[10];
    struct iovec iov[3];
    static int fd[2] = {-1, -1};

    if (fd[0] < 0) { // fork and exec server
        if (fd_pipe(fd) < 0) { // socketpair(), 全双工管道
            err_ret("fd_pipe error");
            return (-1);
        }

        if ((pid = fork()) < 0) {
            err_ret("fork error");
            return (-1);
        } else if (pid == 0) { // child use fd[1]
            close(fd[0]);
            // printf("parent process run\n");
            if (fd[1] != STDIN_FILENO &&
                dup2(fd[1], STDIN_FILENO) != STDIN_FILENO)
                    err_sys("dup2 failed");
            if (fd[1] != STDOUT_FILENO &&
                dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
                    err_sys("dup2 failed");
            if (execl("./opend", "opend", (char*)0) < 0) // 执行服务器程序
                err_sys("execl error:%s", strerror(errno));
            // 父进程的标准输出重定向到了管道,所以这里没有打印这个 log
            // printf("parent pid:%d\n", getpid());
        }
        // else {} // parent use fd[0]
        close(fd[1]);
    }
    // parent
    // format: open <pathname> <openflag>
    sprintf(buf, " %d", oflag); // oflag to ASCII
    iov[0].iov_base = CL_OPEN" "; // string concatenation
    iov[0].iov_len = strlen(CL_OPEN) + 1;
    iov[1].iov_base = name;
    iov[1].iov_len = strlen(name);
    iov[2].iov_base = buf;
    iov[2].iov_len = strlen(buf) + 1;
    len = iov[0].iov_len + iov[1].iov_len + iov[2].iov_len; // gather write
    printf("client send a msg:\n");
    if (writev(fd[0], &iov[0], 3) != len) {
        err_ret("writev failed");
        return (-1);
    }

    return (recv_fd(fd[0], write));
}