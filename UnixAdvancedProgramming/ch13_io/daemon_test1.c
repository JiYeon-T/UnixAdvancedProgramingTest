#include "../common/apue.h"
#include <pthread.h>
#include <time.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>

// syslog
void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void closelog(void);
int setlogmask(int maskptr);

static void daemonize(const char *cmd);


void daemon_process_test(void)
{
    daemonize("./test");

    return;
}

#define LOCKFILE        ("/var/run/daemon.pid")
#define LOCKMODE        (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)


// extern int lockfile(int)

int lockfile(int fd)
{
    return 0;
}

// 守护进程的编程规则:1,2,3...
static void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    // 设置文件创建的权限屏蔽字
    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        err_quit("can't get file limib:%s", cmd);
    
    // 成为一个没有中断的会话首进程
    if ((pid = fork()) < 0)
        err_ret("create process failed")
    else if(pid != 0) { // 创建后父进程退出
        exit(0);
    }
    setsid(); // 创建会话

    // 确保不会为该进程分配终端
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        err_quit("Can't ignore SIGHUP");
    
    if ((pid = fork()) < 0)
        err_ret("create process failed")
    else if (pid != 0)
        exit(0);

    // 修改当前工作目录为根目录
    if (chdir("/") < 0)
        err_quit("change woring directory failed");
    
    // 关闭所有打开的文件描述符, 防止文件系统无法卸载
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; ++i)
        close(i);

    // 修改 stdin, stdout, stderr 输出到 /dev/null
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    //初始化 log 文件
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file desc:%d %d %d\n", fd0, fd1, fd2);
        exit(1);
    }
}


// 文件和记录锁


int alread_runing(void)
{
    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "can't open %s, errno:%s\n", LOCKFILE, strerror(errno));
        exit(1);
    }

    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "can't lock file:%s %s\n", LOCKFILE, strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return (0);
}

// 1. 守护进程启动一般都在 /etc/rcs.* 或者 /etc/init.d 的启动脚本中进行启动
// 2. 启动后会保存一个 /var/run/daemon.pid 文件, 使用文件和记录锁实现单例模式
// 3. 配置文件在 /etc/daemon.conf 中
// 4. 修改修改配置文件后守护进程要可以 respawn, 守护进程要接收 SIGHUP 信号用于修改配置文件后重启守护进程


// 修改配置文件后重启守护进程
sigset_t mask2;
static void reread(void);
void sigterm(int signo);
void sighup(int signo);
static void thred_fun2(void *arg);

void reread_config_test1(int argc, char *argv[])
{
    int err;
    pthread_t tid;
    char *cmd;
    struct sigaction sa;

    if ((cmd = strrchr(argv[0], '/')) == NULL) {
        cmd = argv[0];
    } else {
        cmd++;
    }

    daemonize(cmd);

    if (alread_runing()) {
        syslog(LOG_ERR, "daemon already running\n");
        exit(1);
    }

    // Handle the signal
    sa.sa_handler = sigterm;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGHUP);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        syslog(LOG_ERR, "can't attch SIGTERN:%s\n", strerror(errno));
        exit(1);
    }

    sa.sa_handler = sighup;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        syslog(LOG_ERR, "can't attch SIGHUP:%s\n", strerror(errno));
        exit(1);
    }

    // 处理守护进程重启流程
    exit(0);
}



// 重新读取配置文件
static void reread(void)
{

}

void sigterm(int signo)
{
    syslog(LOG_INFO, "got sigterm exit\n");
    exit(0);
}

void sighup(int signo)
{
    syslog(LOG_INFO, "got sighup, read configuration\n");
    reread();
}

static void thred_fun2(void *arg)
{
    int err, signo;

    for (;;) {
        err = sigwait(&mask2, &signo);
        if (err != 0) {
            syslog(LOG_ERR, "sigwait failed\n");
            exit(1);
        }

        switch (signo) {
            case SIGHUP:
                syslog(LOG_INFO, "Re-reading configuration file\n");
                reread();
                break;
            case SIGTERM:
                syslog(LOG_INFO, "got sigterm, exit\n");
                return;
            default:
                syslog(LOG_WARNING, "unexpected signal:%d\n", signo);
                break;
        }
    }
}

// 对与所有执行程序, 不需要的描述符设置执行时关闭
int set_cloexec(int fd)
{
    int val;

    if ((val = fcntl(fd, F_GETFD, 0)) < 0) 
        return (-1);

    val |= FD_CLOEXEC;

    return (fd, F_SETFD, val);
}
