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


#define SYSLOG_TEST             0
#define DAEMON_PROCESS_TEST     1
#define MAXFD                   64 // 进程打开的所有文件描述符最大值
#define CONFIG_DAEMON_INETD_TEST       0


static int daemon_process; // 守护进程运行标志

// 守护进程的启动方法:

// 由于守护进程没有控制终端,所以当有事件发生的时候,他们的有输出消息的某种方法可用 -> syslogd 守护进程, 
// 通过 syslog 输出
extern void syslog(int priority, const char *message, ...);
extern void openlog(const char *ident, int options, int facility);
extern void closelog(void);


static void syslog_test(void);
static int daemon_init(const char *pname, int facility);
static int Tcp_listen(const char *hostname, const char *service, socklen_t *addrlenp);
void daytime_server(int argc, char *argv[]);
void daemon_inetd(const char *pname, int facility);
int daemon_inetd_test(int argc, const char *argv[]);


static char daytime_log[32] = "daytime";


int main(int argc, char *argv[])
{
#if SYSLOG_TEST
    syslog_test();
#endif

#if DAEMON_PROCESS_TEST
    daytime_server(argc, argv);
#endif

#if CONFIG_DAEMON_INETD_TEST
    daemon_inetd_test(argc, argv);
#endif
}

/**
 * @brief 默认情况下，syslog会将消息写入到/var/log/syslog文件中，其中包括LOG_USER产生的消息。
 * 但是，如果你想让LOG_USER消息被单独保存到一个文件中，你可以通过配置rsyslog（大多数现代Linux发行版使用rsyslog作为syslog的后端）来实现。 
 * syslog 默认配置文件: /etc/rsyslog.d/50-default.conf
 * 
 * 可以编辑 /etc/rsyslog.conf 修改 sylog 配置,例如将 log 文件保存到其他位置
 * 修改后重启服务:
 * sudo systemctl restart rsyslog
 * 或者，如果你使用的是SysV init系统：
 * sudo service rsyslog start
 * 
*/
static void syslog_test(void)
{
    char *from = "a";
    char *to = "b.backup";
    static char *indent = "test_module"; // indent 不能使用栈内存

    // 当前默认配置下, /etc/rsyslog.d/50-default.conf user log 没有打开
    // openlog(indent, LOG_PID | LOG_PERROR | LOG_NDELAY, LOG_USER);
    // 使用 LOG_KERN 进行测试, /var/log/syslog 中可以正常打印日志
    openlog(indent, LOG_PID | LOG_PERROR | LOG_NDELAY, LOG_KERN);
    syslog(LOG_EMERG, "fun:%s", __func__);
    syslog(LOG_ALERT, "fun:%s", __func__);
    syslog(LOG_INFO, "fun:%s", __func__);
    syslog(LOG_DEBUG, "fun:%s", __func__);
    rename(from, to);
    // %m 替换成与当前 errno 对应的错误消息, 类似于: strerror(errno)
    syslog(LOG_EMERG, "rename %s->%s error:%m", from, to);
    closelog();
}

/**
 * @brief 将当前进程变成一个守护进程
 *        TODO: 没整明白
 * 
 * 我们指出,既然守护进程在没有控制终端的环境下运行,他绝不会收到来自内核的 SIGHUP 信号.
 * 许多守护进程因此把这个信号作为来自系统管理员的一个通知,表示其配置文件已发生改动,守护进程应该读入其配置文件.
 * 因此,这些信号可以安全的作为系统管理员的通知手段,指出守护进程应作出反应的某种变动已经发生.
 * 
 * @param[in] pname syslog tag
 * @param[in] pname syslog facility
*/
static int daemon_init(const char *pname, int facility)
{
    int i;
    pid_t pid;
    int fd0, fd1, fd2;

    if ( (pid = Fork()) < 0)
        return (-1);
    else if (pid != 0) // 退出父进程
        _exit(0);

    // child1 continues

    // TODO: 会话首进程 <Unix 环境高级编程>
    if (setsid() < 0) // become a session leader
        return (-1);

    Signal(SIGHUP, SIG_IGN); // 忽略该信号

    // 再次调用 fork() 确保本守护进程将来即使打开了一个控制终端时,也不会自动获得控制终端
    // 该终端也不会成为
    if ( (pid = Fork()) < 0) {
        return (-1);
    } else if (pid != 0) { // child1 退出
        _exit(0);
    }

    // child2 continus
    daemon_process = 1;

    chdir("/"); // 修改进程的工作目录

    // 关闭所有打开的文件描述符:
    for (int i = 0; i < MAXFD; ++i)
        Close(i);
    
    // 重定义 stdin, stdout, stderr to /dev/null
    // 避免异常日志打印到最开始打印的其他描述符
    fd0 = open("/dev/null", O_RDONLY);
    fd1 = open("/dev/null", O_RDWR);
    fd2 = open("/dev/null", O_RDWR);
    
    openlog(pname, LOG_PID | LOG_PERROR | LOG_NDELAY, facility);

    return (0);
}


/**
 * @brief 作为守护进程运行的时间获取服务器成嗯序
 *        sudo ./daytime_server_daemon daytime
*/
void daytime_server(int argc, char *argv[])
{
    int listenfd, connfd;
    socklen_t len;
    char buff[MAXLINE];
    struct sockaddr cliaddr;
    socklen_t cliaddrlen;
    time_t now;
    struct tm *tm;

    if (argc < 2 || argc > 3)
        err_quit("error, usage ./server <service>");

    // 调用 daemon_init 后所有消息都输出到 syslog, 不再有标准输入/输入和控制终端可用
    // OLD:对于 linux 主机 LOG_USER 日志保存在 /var/adm/message 文件(设施为 LOG_USER的消息都发送到该文件)
    LOG_I("Now becom a dameon process");
    // daemon_init(daytime_log, LOG_USER);
    // Q:这里为什么 /var/log/syslog 中没有之后的日志了??
    // A:LOG_I 调用的是 Printf() 当然打印不出来, 守护进程中需要调用 syslog() 打印日志.
    daemon_init(daytime_log, LOG_KERN);

    if (argc == 2)
        listenfd = Tcp_listen(NULL, argv[1], NULL);
    else if (argc == 3)
        listenfd = Tcp_listen(argv[1], argv[2], NULL);

    LOG_I("ipv4 tcp server started.");
    syslog(LOG_ALERT, "ipv4 tcp server started(syslog).");

    for (;;) {
        len = sizeof(cliaddr);
        connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
        LOG_I("connection from %s fd:%d", sock_ntop(&cliaddr, cliaddrlen), connfd);
        syslog(LOG_ALERT, "connection from %s fd:%d", sock_ntop(&cliaddr, cliaddrlen), connfd);
        
        bzero(buff, sizeof(buff));
        // len = read(connfd, buff, sizeof(buff));
        // buff[len] = '\0';
        // LOG_I("recv:%s", buff);
        now = time(NULL);
        tm = gmtime(&now);
        snprintf(buff, sizeof(buff), "%s", asctime(tm));
        Write(connfd, buff, strlen(buff) + 1);
        Close(connfd);
    }
}



/**
 * @brief tcp 服务端接口
 * 
 * @param[in]  hostname 主机名
 * @param[in]  service 服务名
 * @param[out] addrlenp
 * @retval
*/
static int Tcp_listen(const char *hostname, const char *service, socklen_t *addrlenp)
{
    int listenfd, n;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE; // 套接字用于被动打开,
    hints.ai_family = AF_INET; /* IPV6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP */ 

    if ( (n = Getaddrinfo(hostname, service, &hints, &res)) != 0) {
        err_quit("tcp_listen Getaddrinfo error for %s %s :%s", hostname, service, gai_strerror(n));
    }
    ressave = res;
    // TODO: 这里是不是有 bug, 如果 getaddrinfo 

    do {
        listenfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0) {/* error, try next one */
            LOG_E("create listen socket failed");
            continue;
        }
        
        Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0) { /* success */
            LOG_I("bind success");
            break;
        }
        Close(listenfd); // bind error, close and try next one
    } while ( (res = res->ai_next) != NULL );

    if (res == NULL)
        err_quit("tcp_listen error for %s, %s", hostname, service);
    
    Listen(listenfd, LISTENQ);

    if (addrlenp)
        *addrlenp = res->ai_addrlen; // return sizeof protocol address length
    
    freeaddrinfo(ressave);

    return (listenfd);
}

/**
 * @brief 相比与 daemon_init 函数, tcp 连接相关的操作被放到了 inetd 守护进程中执行,
 *        所需要做的是修改 inetd 配置文件 /etc/inetd.conf 添加想要被监听的服务
 * 
 *        inetd 是 BSD 平台的一个守护进程
 *
 * @param[in] panme
 * @param[in] facility
*/
void daemon_inetd(const char *pname, int facility)
{
    daemon_process = 1;
    openlog(pname, LOG_PID, facility);
}


/**
 * @brief inetd 监听到 TCP/UDP 连接后执行该程序编译生成的可执行文件,执行完毕即断开连接
 * 
 * @param[in]
 * @param[in]
*/
int daemon_inetd_test(int argc, const char *argv[])
{
    socklen_t len;
    struct sockaddr *cliaddr;
    char buff[MAXLINE];
    time_t ticks;

    daemon_inetd(argc, argv);

    cliaddr = Malloc(sizeof(struct sockaddr_storage));
    len = sizeof(struct sockaddr_storage);

    // 0 表示已经建立连接的套机子描述符
    Getpeername(0, cliaddr, len);
    LOG_I("connection from %s", sock_ntop(cliaddr, len));

    ticks = time(NULL);
    snprintf(buff, sizeof(buff), "%.24s", ctime(&ticks));
    Write(0, buff, strlen(buff));

    Close(0); // 执行完毕即断开连接
    exit(0); // 退出子进程

}