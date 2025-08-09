#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"


#define ECHO_SERV_PORT                  9999 // echo 服务测试端口
#define CHILD_PROCESS_ZOMBIE_TEST       0 // 子进程变成僵尸线程测试
#define ECHO_SERVER_SUM_TEST                 0 // 求和服务器:服务器端求和后回复给客户端
#define TRANS_DATA_STR_FORMAT           1 // 使用字符串格式交换数据
#define TRANS_DATA_BINARY_FORMAT        0 // 使用二进制格式交换数据
#define CLIENT_CONNECTED_BEFORE_ACCEPT_TEST  0 // 在服务器调用 accept 前 tcp 连接就已经完成, 一般出现在较忙的 web 服务器

static void str_echo(int sockfd);
static void sig_child_handler(int signo);
static void str_sum(int sockfd);
static void bin_sum(int sockfd);


// 多进程并发服务器的框架
#if 0
int main(int argc, char *argv[])
{
    pid_t pid;
    int listenfd, connfd;
    struct sockaddr_t sockaddr;

    Bind(listenfd, ...);
    Listen(listenfd, LISTENQ);

    for (;;) {
        connfd = Accept(listenfd,...);
        if ((pid = Fork()) == 0) { // child
            Close(listenfd); // child close listening socket
            doit(connfd);
            Close(connfd) /* done with this client */;
            exit(0); // child terminate
        }
        Close(connfd); // parent close connection fd:文件描述符的引用计数-1
    }
}
#endif

// tcp 反射服务器(并发服务器)
int main(int argc, char *argv[])
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char buf[MAXLINE];

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

    Signal(SIGCHLD, sig_child_handler);

    LOG_D("server started");
    for (;;) {
#if CLIENT_CONNECTED_BEFORE_ACCEPT_TEST
        // Linux 上测试竟然没有出现错误..
        // 一般会返回 ECONNABORTED,服务器忽略该错误,重新 accept 即可
        sleep(5);
        LOG_D("start accept");
#endif
        clilen = sizeof(cliaddr);
        if ((connfd = Accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) < 0 ) {
            if (errno == EINTR) { // 为了提升系统的可移植性, 在不同的 UNIX 平台上, 对于中断的系统调用是否重新启动处理不同
                // 实测:
                LOG_I("unwanted interupt error:%s", strerror(errno));
                continue;
            } else {
                err_quit("accept error:%s", strerror(errno));
                break;
            }
        }
        LOG_D("conn fd:%d", connfd); // TODO:为什么有多个客户端连接时,拿到的 connfd 都是 4

        if ((childpid = Fork()) == 0) { // child process
            struct sockaddr_storage server_info, client_info;
            socklen_t slen, clen;
            struct sockaddr_in *server, *client;

            slen = sizeof(server_info);
            clen = sizeof(client_info);
            Getsockname(connfd, (struct sockaddr *)&server_info, &slen);
            Getpeername(connfd, (struct sockaddr *)&client_info, &clen);
            server = (struct sockaddr_in *)&server_info;
            client = (struct sockaddr_in *)&client_info;
            Inet_ntop(AF_INET, (const void *)&client->sin_addr, buf, sizeof(buf));
            LOG_I("connection from %s port:%d", buf, ntohs(client->sin_port));

            Close(listenfd); // child close listening socket

#if TRANS_DATA_STR_FORMAT
#if ECHO_SERVER_SUM_TEST
            str_sum(connfd);
#else
            str_echo(connfd); // server process
#endif /* ECHO_SERVER_SUM_TEST */
#elif TRANS_DATA_BINARY_FORMAT
            bin_sum(connfd);
#else

#endif /* TRANS_DATA_STR_FORMAT */
            Close(connfd); // close connfd after processing finish
            LOG_I("child process exit connfd:%d", connfd);
            exit(0); // terminate child
        }
    
        /* parent process */
        Close(connfd); // parent close connfd
    }

    // 进程退出:会关闭所有已打开的文件描述符
}

static void str_echo(int sockfd)
{
    ssize_t n;
    char buff[MAXLINE];

again:
    while ((n = read(sockfd, buff, sizeof(buff))) > 0) {
        LOG_D("server recv from[%d] len:%d data:%s", sockfd, n, buff);
        Writen(sockfd, buff, n); // echo
        memset(buff, 0, sizeof(buff));
    }
    if (n < 0 && errno == EINTR) {
        LOG_I("unwanted interrupt, error:%s", strerror(errno));
        goto again;
    } else if (n < 0) {
        err_sys("server read error");
    } else { // n == 0, 客户关闭程序后, 服务端收到客户发送的 FIN 分节后, read 将返回 0
        // NOTE:服务器异常退出也会走到这里
        LOG_I("read len:%d", n);
    }
}

/**
 * @brief 处理僵尸进程的可移植的方法就是:捕获 SIGCHLD, 并调用 wait()/waidpid()
*/
static void sig_child_handler(int signo)
{
    pid_t pid;
    int exit_status;

#if CHILD_PROCESS_ZOMBIE_TEST
    // 1.wait(), 如果进程结束了但是没有调用 wait()/waitpid() 则会变成`僵尸进程`
    // wait 操作系统会释放与该子进程相关的资源(父进程会获取子进程的信息:cpu时间, 内存是用量,等)
    // 2.如果一个进程结束后,其有子进程处于 zombie 状态,则所有僵尸子进程的 ppid 变为1(init 进程)
    // 继承这些子进程的 init 进程将清理他们
    // 3.如果僵尸进程不处理,会一直占用内核中的空间.直接系统内存耗尽.无论何时 fork 子进程
    // 都要 wait() 他们.
    // 4.处理僵尸进程可移植的方法就是捕获 SIGCHLD 并调用 wait/waitpid;
    pid = wait(&exit_status); // 如果没有子进程退出,wait 将阻塞
    
    // NOTE:在信号处理函数中调用类似 printf() 这样的 IO 函数是不合适的
    // 这里仅测试使用
    printf("child %d terminated\n", pid);
    if (WIFEXITED(exit_status)) {
        printf("exited, status=%d\n", WEXITSTATUS(exit_status));
    } else if (WIFSIGNALED(exit_status)) {
        printf("killed by signal %d\n", WTERMSIG(exit_status));
    } else if (WIFSTOPPED(exit_status)) {
        printf("stopped by signal %d\n", WSTOPSIG(exit_status));
    } else if (WIFCONTINUED(exit_status)) {
        printf("continued\n");
    }
#else 
    // NOTE:这里必须使用循环调用 waitpid(), 因为 "UNIX 中信号是不会排队的",
    // 当多个客户端同时终止时, wait() 只能处理一个服务器子进程, 其余的子进程都变成僵尸进程
    // 参数 -1 表示等待第一个终止的子进程
    // 参数 WNOHANG :告诉内核在没有子进程终止的情况下不要阻塞
    while ((pid = waitpid(-1, &exit_status, WNOHANG)) > 0) {
        printf("child %d terminated\n\n", pid);
        if (WIFEXITED(exit_status)) { // 子进程正常终止
            printf("exited, status=%d\n", WEXITSTATUS(exit_status));
        } else if (WIFSIGNALED(exit_status)) { // 子进程由某个信号杀死
            printf("killed by signal %d\n", WTERMSIG(exit_status));
        } else if (WIFSTOPPED(exit_status)) { // 子进程由作业控制停止
            printf("stopped by signal %d\n", WSTOPSIG(exit_status));
        } else if (WIFCONTINUED(exit_status)) { // 子进程继续开始工作
            printf("continued\n");
        }
    }
#endif

    return;
}

/**
 * @brief 计算客户端输入两个值的和, "客户和服务端传输的数据为字符串格式"
 *        需要在单独的子进程中执行, 因为会有死循环
 *        这个程序大大端/小端的机器上都运行的很好
*/
static void str_sum(int sockfd)
{
    long arg1, arg2;
    ssize_t n;
    char line[MAXLINE];

    for (;;) {
        if ((n = readline_v1(sockfd, line, sizeof(line))) == 0) //connection closed by other end
            return;
        LOG_D("server recv from[%d] len:%d %s", sockfd, n, line);
        if (sscanf(line, "%ld %ld", &arg1, &arg2) == 2)
            snprintf(line, sizeof(line), "%ld\n", arg1 + arg2);
        else
            snprintf(line, sizeof(line), "input error\n");
        Writen(sockfd, line, strlen(line));
    }
}

/**
 * @brief 使用二进制格式交换数据
 *        必须在子进程中调用
*/
static void bin_sum(int sockfd)
{
    ssize_t n;
    struct args arg;
    struct result result;


    for (;;) {
        if ((n = Readn(sockfd, &arg, sizeof(arg))) == 0)
            return; // connect closed by other end
        LOG_D("server recv from %d args:%d %d", sockfd, arg.arg1, arg.arg2);
        result.sum = arg.arg1 + arg.arg2;
        Writen(sockfd, &result, sizeof(result));
    }
}

