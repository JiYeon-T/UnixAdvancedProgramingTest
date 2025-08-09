#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"

#include <sys/select.h>
#include <math.h>


#define ECHO_SERV_PORT                  9999 // echo 服务测试端口
#define MULTI_CLIENT_TEST               0 // 多个客户端连接, 服务端派生多个子进程测试
#define SERVER_INTERRUPT_EXIT_TEST      0 // 服务器程序崩溃退出时客户端的处理
#define TRANS_DATA_STR_FORMAT           0 // 使用字符串格式交换数据
#define TRANS_DATA_BINARY_FORMAT        0 // 使用二进制格式交换数据
#define MUTLI_IO_TEST                   0 // 使用 IO 多路复用
#define BATCH_INPUT_TEST                1 // 批量输入/输出, 输入/输出重定向到文件

static void str_cli(FILE *fp, int socketfd);
static void sig_pipe_handler(int signo);
static void str_cli_v2(FILE *fp, int socketfd);
static void str_cli_v3(FILE *fp, int socketfd);
static void str_cli_v4(FILE *fp, int socketfd);



#if MULTI_CLIENT_TEST
int main(int argc, char *argv[])
{
    int i, sockfd[5];
    struct sockaddr_in servaddr;
    int connfd;


    if (argc != 2)
        err_quit("usage: client <IP address>");

    for (i = 0; i < 5; ++i) {
        sockfd[i] = Socket(AF_INET, SOCK_STREAM, 0);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(ECHO_SERV_PORT);
        Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
        connfd = connect(sockfd[i], (struct sockaddr*)&servaddr, sizeof(servaddr));
        LOG_I("connect [%d] from", connfd);
    }

    str_cli(stdin, sockfd[0]);
    Close(sockfd[0]); // 这里仅主动关闭第一个 socket, 其余的在进程退出后由内核关闭

    exit(0);
}
#else
int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in servaddr;
    char buff[MAXLINE];

    if (argc != 2)
        err_quit("usage: client <IP address>");

#if SERVER_INTERRUPT_EXIT_TEST
    Signal(SIGPIPE, sig_pipe_handler);
#endif

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    // Connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    LOG_I("client connect success\n");

#if TRANS_DATA_STR_FORMAT
    str_cli(stdin, sockfd);
#elif TRANS_DATA_BINARY_FORMAT
    str_cli_v2(stdin, sockfd);
#elif MUTLI_IO_TEST
    str_cli_v3(stdin, sockfd);
#elif BATCH_INPUT_TEST
    str_cli_v4(stdin, sockfd);
#else

#endif /* TRANS_DATA_STR_FORMAT */
    Close(sockfd); // 关闭套接字, 这里可以不加, 进程退出时会关闭已打开的描述符
    LOG_I("client %d exit ppid:%d", getpid(), getppid());
    exit(0);
}
#endif

/**
 * @brief 回射服务器:从标准输入读的发送到 socket, 从 socket 读的发送到 stdout
 *        客户端退出方式:输入 ^D (EOF) fgets() 将返回空指针,
*/
static void str_cli(FILE *fp, int socketfd)
{
    char sendline[MAXLINE], recvline[MAXLINE];
    ssize_t n;

    while (Fgets(sendline, sizeof(sendline), fp) != NULL) {
#if SERVER_INTERRUPT_EXIT_TEST
        // (1) 服务器进程终止:
        // 手动杀死服务器程序(模拟服务器进程崩溃场景), $kill -9 cliend_pid
        // 服务器客户端两对套接字连接时,杀掉服务器程序, 服务器发送 FIN, 客户端返回 ACK, 服务器端描述符进入半关闭状态
        // 但是客户进程阻塞在 fgets(), 进程没有退出因此并没有 close() 对应的描述符, 导致客户端没有发送 FIN
        // server_socket 处于: FIN_WAIT2 状态
        // client_socket 处于: CLOSE_WAIT 状态
        // 但是由于客户端线程阻塞在 read() 函数, 导致无法处理
        // 此时, 客户端向服务器发送数据时, 服务器会返回 RST 分节,
        // 如果客户端忽略这个 RST 继续向该套接字写数据:当一个进程向已经收到 RST 分节的套接字写数据时, 
        // 内核会向该进程发送 SIGPIPE 信号, 该信号的默认行为是终止进程
        // 如果客户端捕获 SIGPIPE 信号, 则进程不会终止, 继续向该套接字写数据时, 会返回 EPIPE 错误
        // (2)服务器主机崩溃:
        // 当客户输入数据,尝试通过 tcp 发送该包数据时,由于无法发送到服务器,协议栈会一直重传该 tcp 分节
        // 客户阻塞在 readline(), 服务器响应以 ETIMEOUT
        // 如果某个路由器发现该主机已经不可达,通过 ICMP 返回该主机不可达,则会返回 EHOSTUNREACH/ENETUNREACH
        // 3)服务器关机
        // init 进程会先给所有进程发送 SITTERM, 然后给所有进程发送 SITKILL
        // 客户端必须使用 select/poll 使得服务器进程一旦终止,客户进程就可以检测到.
        // TODO: (1) SO_KEEPALIVE, (2)服务器心博函数
        n = Writen(socketfd, sendline, 1); // 返回服务器发送的 RST 分节
        if (n < 0) {
            LOG_E("write failed error:%s", strerror(errno));
        }
        sleep(1);
        n = Writen(socketfd, sendline+1, strlen(sendline) - 1); // 收到内核发送的 SIGPIPE 信号
        if (n < 0) {
            LOG_E("write failed error:%s", strerror(errno));
        }
#else
        Writen(socketfd, sendline, strlen(sendline));
#endif
        if ((n = readline_v1(socketfd, recvline, sizeof(recvline))) == 0) { // 这里客户端必须读到换行符, 因此服务端返回结果时必须添加 '\n'
            // 客户端进程退出
            err_quit("client exit, reason:server terminated prematurely, error:%s", strerror(errno));
        }
        LOG_D("client recv[%d]:%s\n", n, recvline);
        Fputs(recvline, stdout);
    }
}

static void sig_pipe_handler(int signo)
{
    printf("SIGPIPE catched\n");
}

/**
 * @brief 服务器客户端之间交换数据为二进制格式,而不是 ASCII 字符串, 
 *      与机器架构有关了, 大小端, long 类型占用的字节数,
 *       客户和服务器运行在不同主机时,可能会出问题
 * 
 * @param[in]  fp 标准输入
 * @param[in]  socketfd 套接字
*/
static void str_cli_v2(FILE *fp, int socketfd)
{
    char sendline[MAXLINE];
    struct args arg;
    struct result result;

    while (Fgets(sendline, sizeof(sendline), fp) != NULL) {
        if (sscanf(sendline, "%ld %ld", &arg.arg1, &arg.arg2) != 2) {
            LOG_E("invalid input:%s", sendline);
            continue;
        }
        Writen(socketfd, &arg, sizeof(struct args)); // 二进制发送
        if (Readn(socketfd, &result, sizeof(result)) == 0)
            err_quit("client:server terminated prematurely");
        LOG_I("result:%ld", result.sum);
    }
}

/**
 * @brief 使用 IO 多路复用监听用户输入以及 socket 输入
 *        主要为了解决客户端阻塞在 fgets() 导致线程收不到服务端崩溃消息(RST分节)的问题
 *        现象:服务端退出后客户端马上退出, 而不需要等到客户端下次发送数据的时候
 *        该函数必须在子进程中调用_有死循环
 * 
 * @param[in]  fp 标准输入
 * @param[in]  socketfd 套接字
*/
static void str_cli_v3(FILE *fp, int socketfd)
{
    int maxfdp1;
    fd_set rset, wset, exceptset;
    char sendline[MAXLINE], recvline[MAXLINE];
    int n;

    FD_ZERO(&rset);
    for (;;) {
        // 同时监听套接字和标准输入
        FD_SET(fileno(fp), &rset);
        FD_SET(socketfd, &rset);
        maxfdp1 = max(fileno(fp), socketfd) + 1;
        n = Select(maxfdp1, &rset, NULL, NULL, NULL);
        /**
         * NOTE:被中断的系统调用,不同 unix 平台对该功能实现不同
         * 从可移植性的角度考虑,如果我们在捕获信号,那么必须做好 select 返回 EINTR 的准备
        */
        if (n < 0 && errno == EINTR) {
            LOG_W("unwanted interrupt reason:%s", strerror(errno));
            continue;
        }

        if (FD_ISSET(socketfd, &rset)) { // socket is readable
            if (readline_v1(socketfd, recvline, sizeof(recvline)) == 0) { // RST 分节
                // NOTE:从标准输入读到 EOF 时客户端进程退出
                err_quit("client:server terminated prematurely");
            }
            // LOG_D("sock read:%s", recvline);
            Fputs(recvline, stdout);
        }

        if (FD_ISSET(fileno(fp), &rset)) { // input is readable
            if (Fgets(sendline, sizeof(sendline), fp) == NULL) { // all done
                LOG_I("input finish,exit");
                return;
            }
            // LOG_D("stdin read:%s", sendline);
            Writen(socketfd, sendline, strlen(sendline));
        }
    }
}

/**
 * @brief 
 *        解决批量输入情况(重定位输入&输出到文件)下输入,输出文件大小不一致问题
 *        原因:socket 有缓冲区,有部分数据还在发往服务器途中/服务器向客户端途中
 *        导致输入传输还没有结束就关闭了客户端(Fgets() 返回 NULL)
 *        解决:从标准输入读取 EOF 后,使用 shutdown 关闭写半部, 而不关闭进程
*/
static void str_cli_v4(FILE *fp, int socketfd)
{
    int maxfdp1;
    fd_set rset, wset, exceptset;
    char sendline[MAXLINE], recvline[MAXLINE];
    int stdin_eof = 0; // 该标志位为 1 才退出进程
    int n;

    FD_ZERO(&rset);
    for (;;) {
        if (stdin_eof == 0)
            FD_SET(fileno(fp), &rset);
        FD_SET(socketfd, &rset);
        maxfdp1 = max(fileno(fp), socketfd) + 1;
        Select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(socketfd, &rset)) { // socket is readable
            if ((n = Read(socketfd, recvline, sizeof(recvline))) == 0) {// RST 分节
                if (stdin_eof == 1) { // normal termination
                    LOG_I("normal exit");
                    return;
                } else {
                    err_quit("client:server terminated prematurely");
                }
            }
            // LOG_D("sock read:%s", recvline);
            Writen(fileno(stdout), recvline, n);
        }

        if (FD_ISSET(fileno(fp), &rset)) { // input is readable
            if ((n = Read(fileno(fp), sendline, sizeof(sendline))) == 0) {// all done
                /* 标准输入读到 EOF 时,关闭 socketfd 的写半部
                client 不能往 server 发送数据,但是 server 仍然可以给 client 发送 */
                stdin_eof = 1;
                Shutdown(socketfd, SHUT_WR); // 关闭套接字的写半部,send FIN
                FD_CLR(fileno(fp), &rset);
                continue;
            }
            // LOG_D("stdin read:%s", sendline);
            Writen(socketfd, sendline, strlen(sendline));
        }
    }
}
