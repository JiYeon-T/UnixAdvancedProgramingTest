#include "../common/apue.h"
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <mqueue.h>
/***************************************************************
 * Macro
 ***************************************************************/
#define NQ          3       // number of queues
#define MAXMSZ      512      // maximum message size
#define KEY         123   // key of first message queue
// #define UNIX_DOMAIN_SOCKET_PATH     "/var/tmp/foo.test"
#define UNIX_DOMAIN_SOCKET_PATH     "./server_sock.test"

extern int socketpair(int domain, int type, int protocol, int sockfd[2]); //创建一对无命名的,相互连接的 unix 域套接字


struct threadinfo {
    int qid;
    int fd;
};

struct msg {
    long msg_type;
    char data[MAXMSZ];
};

/**
 * @fun 起到全双工管道的作用
 *      两个描述符都可以用于读写
 *      1.socketpair() 创建的 unix 域套接字,只能在同一个进程内使用(可以跨线程)，
 *      类似于无名管道 pipe(), 但是 pipe() 是半双工管道;
 * 
 * 
 * 
*/
int fd_pipe(int fd[2])
{
    return (socketpair(AF_UNIX, SOCK_STREAM, 0, fd));
}

/*
"借助 UDS 轮询 XSI 消息队列:"
消息队列不能配合 poll()/select() 使用, 因为他们是监听文件描述符的。套接字是和文件描述符相关的。

实现原理:消息到达时,套接字接收通知.对每个消息队列使用一个线程, 线程会在 msgrecv() 的地方阻塞, 
收到数据后写入套接字的一端
应用程序使用 poll()/select() 监听套接字的另一端，这样就可以配合 poll()/select()使用

缺点:
(1) 额外创建一个线程; 
(2) 数据拷贝两次;
优点:
使得 XSI 消息队列的使用更加容易
socketpair() 本身不支持跨进程通信(无名), 通过消息队列使得支持 ? 或者跨进程就应该使用有名 UDS
应用场景:
*/

/**
 * @brief 线程入口函数
 * 
 * @param[in] arg struct threadinfo 类型
*/
void *helper(void *arg)
{
    int n;
    struct msg m;
    struct threadinfo *p_thread_info = arg;

    //TODO:
    // for(;;) 和 while(1) 写法有区别?
    for (;;) {
        memset(&m, 0, sizeof(m));
        //阻塞消息队列
        // TODO:
        // mq_receive 和 msgrcv() 的区别 /??
        if ((n = msgrcv(p_thread_info->qid, &m, MAXMSZ, 0, MSG_NOERROR)) < 0)
            err_sys("msgrcv error");
        //写套接字的一端
        if (write(p_thread_info->fd, m.data, n) < 0)
            err_sys("write error");
    }
}

/**
 * @brief 接收函数
 *        主线程阻塞在 poll()
 *        接受子线程阻塞在 msg_recv()/msgrcv()
 *        ./recv
 * 
*/
void socketpair_recv_test(void)
{
    int n, i, err;
    int fd[2];
    int qid[NQ];
    struct pollfd pfd[NQ];
    struct threadinfo thread_info_list[NQ];
    pthread_t thread_list[NQ];
    char buf[MAXMSZ];

    for (i = 0; i < NQ; ++i) {
        // N 个消息队列
        // 消息队列的 base 是 key
        // 第 i 个队列的 key 是: KEY+i
        if ((qid[i] = msgget((KEY + i), IPC_CREAT|0666)) < 0)
            err_ret("msgget failed");
        printf("queue ID %d is %d\n", i, qid[i]);
        
        // 使用数据报套接字而不是流套接字,保持消息边界,保证一次只能读取一条消息
        // socketpair 使用 DGRAM, 保证了数据是一条一条读取的
        if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fd) < 0)
            err_ret("create socketpair failed");
        
        pfd[i].fd = fd[0]; // 作为读端, 全双工, 都可以同时读写
        pfd[i].events = POLLIN;
        thread_info_list[i].qid = qid[i];
        thread_info_list[i].fd = fd[1]; // 作为写端
        if ((err = pthread_create(&thread_list[i], NULL, helper, &thread_info_list[i])) != 0)
            err_ret("pthread_create error");
    }

    // 使用 SOCK_STREAM 会导致每次读取到的数据长度不一致
    printf("start poll:\n");
    for (;;) {
        if (poll(pfd, NQ, -1) < 0) // 一直阻塞
            err_ret("poll error errno:%d", errno);
        
        for (i = 0; i < NQ; ++i) {
            if (pfd[i].revents & POLLIN) {
                if ((n = read(pfd[i].fd, buf, sizeof(buf))) < 0)
                    err_sys("read failed");
                buf[n] = 0; // '\0'
                printf("queue id:%d message:%s\n", qid[i], buf);
            }
        }
    }
    exit(0);
}

/**
 * @brief 发送函数
 *        ./send 123 abc
*/
void socketpair_send_test(int argc, char *argv[])
{
    key_t key;
    long qid;
    size_t nbytes;
    struct msg m;

#if 0
    key = KEY;
#else
    if (argc != 3)
        err_quit("usage:sendtest <KEY> <message>");
    key = strtol(argv[1], NULL, 0);
#endif
    // 支持不同进程访问, 
    // 通过 msgget 得到 qid, 其他进程通过 qid 读写消息队列
    if ((qid = msgget(key, 0)) < 0)
        err_quit("can't open message queue key:%d\n", key);

    memset(&m, 0, sizeof(m));
    strncpy(m.data, argv[2], MAXMSZ-1);
    nbytes = strlen(m.data);
    m.msg_type = 1;
    if (msgsnd(qid, &m, nbytes, 0) < 0)
        err_sys("can't send message");
    exit(0);
}

/**
 * @brief "命名UNIX域套接字"
 *        将一个地址绑定到 UNIX 域套接字(命名UNIX域套接字, 这里是:UNIX_DOMAIN_SOCKET_PATH)
 *        同一台电脑通过 unix 域套接字进程间通信的方式
 * 
 * @ret
 * 
*/
void unix_socket_test1(void)
{
    int fd, addr_len;
    // unix 地址结构, 与 internet 套接字地址结构不同, #include <sys/un.h>
    struct sockaddr_un un;
    const char * socket_path = "foo.socket";

    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, socket_path);
    // 不同实现的 unix 操作系统, unix 地址结构体实现不同
    // 因此计算 addr_len 的方法变成下面这种形式, 
    //直接使用 sizeof(struct sockaddr_un) 也可以, 但是这会导致每一个套接字地址都是 sizeof(struct sockaddr_un) 位, 可用的地址总数减少?
    // addr_len = sizeof(struct sockaddr_un);
    addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    printf("addrlen:%d\n", addr_len);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        err_exit("socket failed");

    if (bind(fd, (struct sockaddr *)&un, addr_len) < 0)
        err_exit("bind failed errno:%d", errno);
    printf("unix domain socket bound\n");

    // 当试图绑定(bind)同一地址时, 如果该文件已经存在, 会绑定失败
    // 程序退出后, 该文件不会自动删除, 因此必须手动删除该文件
    unlink(socket_path);

    exit(0);
}

/**
 * @brief 有名域套接字服务器进程
 *        同一台电脑通过 unix 域套接字进程间通信的方式
 *        创建 socket() -> bind() -> listen() -> accept()
 * 
*/
void unix_socket_server_test2(void)
{
    int listen_fd, fd;
    uid_t user_id;
    int n;
    char buff[2048]  = {0};

    printf("serv_listen:%d\n", getpid());
    if ((listen_fd = serv_listen(UNIX_DOMAIN_SOCKET_PATH)) < 0) 
        err_ret("listen failed:%d\n", listen_fd);

    printf("serv_accept:%d\n", getpid());
    if ((fd = serv_accept(listen_fd, &user_id)) < 0)
        err_ret("accept failed:%d\n", fd);
    
    printf("server[%ld] start read\n", getpid());
    while (1) {
        bzero(buff, sizeof(buff));
        n = read(fd, buff, 2048);
        // buff[n] = '\0';
        // printf("read len:%d data:%s\n", n, buff);
        printf("read len:%d data:%c %d\n", n, buff[0], buff[0]);
    }
}

/**
 * @brief 命名 UNIX 域套接字
 *        创建 socket() -> bind() -> connect()
 * 
*/
void unix_socket_client_test2(void)
{
    int fd;
    char buff[1024] = {0};
    char ix = 0;
    int nread;

    printf("client[%ld] conn:\n", getpid());
    if ((fd = cli_conn(UNIX_DOMAIN_SOCKET_PATH)) < 0)
        err_ret("connect failed\n");

    printf("client[%ld] start read:\n", getpid());
    // 死循环:因为 uint8_t:0-255
    while(1) {
        ++ix;
        memset(buff, ix, sizeof(buff));
        // snprintf(buff, 512, "data ix:%d\n", ++ix);
        // printf("client send:%s\n", buff);
        printf("client[%d] send len:%d data:%c %d\n", getpid(), sizeof(buff), buff[0], buff[0]);
        write(fd, buff, sizeof(buff));
        sleep(2);
        // bzero(buff, sizeof(buff));
        // nread = read(fd, buff, 1024);
        // printf("client read:%d\n", nread);
    }
    exit(0);   
}

/**
 * @brief UDS 文件描述符测试 服务器发送文件描述符
 * 
 * 
*/
void send_fd_server_test1(void)
{
    int listen_fd, socket_fd;
    uid_t user_id;
    int ret;

    if ((listen_fd = serv_listen(UNIX_DOMAIN_SOCKET_PATH)) < 0) 
        err_ret("listen failed:%d\n", listen_fd);
    
    printf("wait for client connect\n");
    if ((socket_fd = serv_accept(listen_fd, &user_id)) < 0) {
        err_ret("accept failed:%d\n", socket_fd);
    }

    //服务器端通过 UDS 发送自己的标准输出进程给到客户端
    ret = send_fd(socket_fd, STDOUT_FILENO);
    ASSERT(ret == 0);

    while (1) {
        printf("server running\n");
        sleep(1);
    }
}

static ssize_t error_process_func(int fd, const void *buff, size_t nread)
{
    int nwrite;

    if ((nwrite = write(fd, buff, nread)) != nread) {
        return -1;
    }

    return 0;
}

/**
 * @brief UDS 文件描述符测试 客户端接收文件描述符
 *        客户端通过接收到的文件描述符读写数据
 * 
*/
void recv_fd_client_test1(void)
{
    int socket_fd;
    int recv_new_fd;
    int ix = 0;
    char buff[256];

    if ((socket_fd = cli_conn(UNIX_DOMAIN_SOCKET_PATH)) < 0)
        err_ret("connect failed\n");

    recv_new_fd = recv_fd(socket_fd, error_process_func);
    ASSERT(recv_new_fd >= 0);

    while (1) {
        snprintf(buff, 256, "String:%d from client", ix++);
        write(recv_new_fd, buff, strlen(buff));
        sleep(1);
    }
}

static void *send_entry1(void *arg)
{
    int nsend;
    int fd = *(int*)arg;
    printf("%s pid:%ld fd:%d\n", __func__, getpid(), fd);

    while (1) {
        nsend = write(fd, "hello", 5);
        printf("[%ld] send len:%d\n", getpid(), nsend);
        sleep(1);
    }
}

static void *recv_entry1(void *arg)
{
    int nrecv;
    int fd = *(int*)arg;
    int buff[256];
    printf("%s pid:%ld fd:%d\n", __func__, getpid(), fd);

    while (1) {
        nrecv = read(fd, buff, 256);
        printf("[%ld] recv len:%d\n", getpid(), nrecv);
        // sleep(1);
    }
}


/**
 * @brief socketpair() 跨线程通信测试
 *        进程内存分布图, 同一进程中的线程共享打开的文件描述符, 例如:标准描述符 STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO
 *       
 * @param[in]
*/
void uds_thread_test(void)
{
    static int fd[2];
    int ret;
    pthread_t th_send, th_recv;

    ret = socketpair(AF_UNIX,SOCK_DGRAM, 0, fd);
    // ASSERT(ret == 0);

    ret = pthread_create(&th_send, NULL, send_entry1, &fd[0]);
    // ASSERT(ret == 0);

    ret = pthread_create(&th_recv, NULL, recv_entry1, &fd[1]);
    // ASSERT(ret == 0);

    // pthread_join(&th_send, NULL);
    // pthread_join(&th_recv, NULL);
    while (1);
    printf("main exit\n");
}
