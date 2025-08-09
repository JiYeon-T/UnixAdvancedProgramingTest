#include "../common/apue.h"

/*
 *	ISO C99 Standard: 7.5 Errors	<errno.h>
 */
#include <errno.h>

/*
 *	ISO C99 Standard: 7.14 Signal handling <signal.h>
 */
#include <signal.h>

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/*
 *	POSIX Standard: 9.2.2 User Database Access	<pwd.h>
 */
#include <pwd.h>

/*
 *	POSIX Standard: 3.2.1 Wait for Process Termination	<sys/wait.h>
 */
#include <sys/wait.h>

/*
 *	ISO C99 Standard: 7.13 Nonlocal jumps	<setjmp.h>
 */
#include <setjmp.h>

/*
 *	ISO C99 Standard: 7.23 Date and time	<time.h>
 */
#include <time.h>

#include <stdlib.h>
#include <stdio.h>

// 进程, 父进程, 进程组, 会话(进程组集合), 作业控制(进程组集合)
// SIGINT:CTRL^C:
// SIGSEGV:硬件异常:/0, 无效内存引用等
// SIGURG:网络上收到了带外数据
// SIGALRM:alarm() 产生的闹钟信号
// SIGPIPE:管道的的读进程已终止后,还有进程写该管道时产生
// SIGCHLD:一个子进程已经终止
// SIGKILL 和 SIGSTOP 不可以被用户捕获,提供给超级用户和和内核终止进程使用
// 大多数信号的默认操作是终止进程


// char *getpass(const char *prompt); // 废弃
// 进程组
pid_t getpgrp(void); // 获取当前进程所属进程组 ID
pid_t getpgid(pid_t pid); // 获取指定进程的进程组 ID
int setpgid(pid_t pid, pid_t pgid); //创建一个进程组/将进程转移到其他进程组(设置进程组ID)
// int setpgrp(pid_t pid); // setpgrp(0) == getpgrp()
pid_t setsid(void); // 创建一个会话
pid_t getsid(pid_t pid); // 获取会话 ID(会话首进程的进程组 ID)
// 会话
pid_t tcgetprrp(int fd); // 返回前台进程组 ID
int tcsetpgrp(int fd, pid_t pgrpid); // 设置前台进程组 ID
pid_t tcgetsid(int fd); // 获取会话首进程 ID

extern int kill(pid_t pid, int signo); // kill 默认传送 SIGTERM 信号
extern int raise(int signo); // 给进程自己发送信号
extern unsigned int alarm(unsigned int seconds); // 为进程注册一个闹钟
extern int pause(void); // 阻塞等待一个信号
void abort(void); // 等效于:raise(SIGABRT);

// signal set
extern int sigemptyset(sigset_t *set);
extern int sigfillset(sigset_t *set);
extern int sigaddset(sigset_t *set, int signo);
extern int sigdelset(sigset_t *set, int signo);
extern int sigismember(const sigset_t *set, int signo);
extern int sigprocmask(int how, const sigset_t *restrict set, sigset_t * restrict oset);
extern int sigpending(sigset_t *set);
// 检查/修改与指定信号相关联的处理和动作, 第二个参数可以带额外的屏蔽的信号列表
extern int sigaction(int signo, const struct sigaction * restrict act,
                        struct sigaction * restrict oact);
int sigsetjmp(sigjmp_buf env, int savemask);
void siglongjmp(sigjmp_buf env, int val); // 恢复 sigsetjmp() 的信号屏蔽字
int sigsuspend(const sigset_t *sigmask); // 挂起进程等待信号
//注册信号时需要使用 SA_SIGINFO 选项才可以获取发送信号时到传入的参数
int sigqueue(pid_t pid, int signo, const union sigval value); // 与 kill() 相似, 发送信号, 
void psignal(int signo, const char *msg);
void psiginfo(const siginfo_t *info, const char *msg);
char *strsignal(int signo);

Sigfunc *signal_m(int signo, Sigfunc *func);
Sigfunc *signal_intr(int signo, Sigfunc *func);



/**
 * @brief 模拟 传统 UNIX 系统用户登录流程
 * 现代 UNIX 使用 PAM (Pluggle Authentication Modules)方案(允许管理元进行配置)
*/
void unix_login_process(int argc, char *argv[])
{
    pid_t pid;
    int ret;
    extern char **environ;

    // step1: 超级用户在 Init 进程中以哦嗯环境 exec getty
    printf("process[%ld](parent:%ld) enter\n", getpid(), getppid());
    if ((pid = fork()) < 0) {
        perror("fork failed");
        return;
    } else if (pid > 0) { // parent
        // let parent run first
    } else { // child
        printf("process[%ld](parent:%ld) enter\n", getpid(), getppid());

        //TODO:需要进程同步
        sleep(1); // let parent run first
        
        //运行成功后, 文件描述符 0, 1 2 就被设置到该设备
        execle("/sbin/getty", "getty", "1", (char*)0, environ);

        //sptep2: 运行 login 程序
        const char username[] = "qz";
        execle("/bin/login", "login", "-p", username, (char*)0, environ);

        // step3: login 程序登录成功后设置用户 ID, 再运行登录用户的 shell
        setuid(username);
        execl("/bin/sh", "-sh", (char*)0);
        
        //step4: 登录成功后还可以进行其他功能, 可以自行添加
        // 打印日期消息, 文件, 检查新邮件等其他任务

        printf("process[%ld](parent:%ld) exit\n", getpid(), getppid());
        exit(0);
    }

    if (waitpid(pid, NULL, 0) != pid) {
        perror("wait pid failed");
    }
    printf("process[%ld](parent:%ld) exit\n", getpid(), getppid());
}

/**
 * @brief 进程组
 * 一个进程只能为自己/自己的子进程设置进程组 ID
 * 需要在 exec 之前调用该函数, 大多数 shell 作业控制中, 父子进程中都设置 group ID, 其中有一个是
 * 冗余的, 主要是为了避险由于父子进程运行顺序先后引起的竞争
 * 
*/
typedef struct _process_info{
    pid_t pid;
    char name[128];
    //other
} process_info_t;
void process_grp_test1(void)
{
    // 多个进程组形成会话
    process_info_t process_pool[10];
    pid_t pid;
    int status;

    printf("process[%ld](parent:%ld) enter\n", getpid(), getppid());
    memset(&process_pool, 0, sizeof(process_pool));

    for (int i = 0; i < 2; ++i) {
        if ((pid = fork()) < 0) {
            perror("fork failed");
            return;
        } else if (pid > 0) {
            setpgid(0, getpid()); // 设置进程组 ID
            process_pool[i].pid = pid;
            snprintf(process_pool[i].name, 128, "child%d", i);
        } else {
            setpgid(0, getppid());
            static int cnt;
            usleep(1000); // let parent run first
            printf("process[%ld](parent:%ld) enter\n", getpid(), getppid());
            while (1) {
                // 父子进程拥有独立的栈空间
                // 所以子进程 1 的数组是这样的 {0, 0,...}
                //子进程2 的数组: {12309, 0, ...}, 多了第一个进程的 pid, 因为是从父进程拷贝过来的
                // if (getpid() == process_pool[0].pid && cnt++ >= 2) {
                //     break;
                // }
                // if (getpid() == process_pool[1].pid && cnt++ >= 5) {
                //     break;
                // }
                printf("%ld running\n", getpid());
                for (int i = 0; i < 10; ++i) {
                    printf("[%d]:%d  ",i, process_pool[i].pid);
                }
                printf("\n");
                sleep(1);
            }
            printf("process[%ld](parent:%ld) exit\n", getpid(), getppid());
            exit(0);
        }
    }

    for (int i = 0; i < 2; ++i) {
        pid = wait(&status);
        printf("[%ld] exit status:%ld\n", pid, status);
        pr_exit(status);
    }
    
    printf("process[%ld](parent:%ld) exit\n", getpid(), getppid());

    return;
}

/**
 * @brief  


*/
void shell_running_process(void)
{
//   $ ps -o pid,ppid,pgid,sid,comm
//   PID  PPID  PGID   SID COMMAND
//   15415  4003 15415 15415 bash
//   15629 15415 15629 15415 ps
    // ps 的父进程是 bash
    // ps 和 shell 位于同一个会话, 两个进程组

// ps -o pid,ppid,pgid,sid,comm
//   PID  PPID  PGID   SID COMMAND
//  4008  4003  4008  4008 bash
// 15185  4008 15185  4008 cat
// 15217  4008 15217  4008 cat
// 15372  4008 15372  4008 ps

}

// orphaned process group
static void sig_hup(int signo)
{
    printf("SIGHUB received, pid = %ld\n", (long)getpid());
}

static void pr_ids(char *name)
{
    printf("%s: pid=%ld, ppid=%ld, pgrp=%ld, tpgrp=%ld\n", 
            name, (long)getpid(), (long)getppid(), (long)getpgrp(), \
            (long)tcgetpgrp(STDIN_FILENO));
    fflush(stdout);
}

int sig_hup_test(void)
{
    char c;
    pid_t pid;

    pr_ids("parent");

    if ((pid = fork()) < 0) {
        err_sys("fork failed");
        return -1;
    } else if (pid > 0) { // parent
        sleep(5); // sleep to let child stop itself
    } else { // child
        // 使用 kill 命令向后台运行的进程发送信号:kill -USR1 7216
        pr_ids("child");
        signal(SIGHUP, sig_hup); // establish signal handler
        // #define	SIGTSTP		20	/* Keyboard stop (POSIX).  */
        kill(getpid(), SIGTSTP); // stop ourself, 子进程停止, 但是并没有退出
        // 5 秒后, 父进程退出后, 子进程成了一个孤儿进程组的成员, 由 init 进程继承
        // POSIX 规定, 这时候父进程需要向子进程发送 SIGHUP -> 然后发送 SIGCONT
        pr_ids("child wakeup:"); // print only if we're continued

        // 变为孤儿进程组后(也是后台进程组), 后台进程组无法读控制终端
        // #define	EIO		 5	/* I/O error */
        if (read(STDIN_FILENO, &c, 1) != 1) {
            err_sys("read err on controlling TTY, errno:%d", errno);
        }
    }
    exit(0);
}

// Note:
// we'd better use sigaction() rather thann signal() 
// cause the difference signal() realization
static void sig_usr_handler(int signo);
void sig_usr_test(void)
{
    if (signal(SIGUSR1, sig_usr_handler) == SIG_ERR) {
        err_sys("can't catch SIGUSR1");
    }
    if (signal(SIGUSR2, sig_usr_handler) == SIG_ERR) {
        err_sys("can't catch SIGUSR2");
    }
    for (;;) 
        pause();
}

static void sig_usr_handler(int signo)
{
    if (signo == SIGUSR1) {
        printf("%d SIGUSR1 received\n", getpid());
        exit(-1);
    } else if (signo == SIGUSR2) {
        printf("%d SIGUSR2 received\n", getpid());
    } else {
        err_quit("unknown signo:%d", signo);
    }
}

static void sig_alrm_handler(int signo);
void sig_alarm_test(void)
{
    struct passwd *ptr;
    int num = 0;

    if (signal(SIGALRM, sig_alrm_handler) == SIG_ERR) {
        err_ret("Can't catch SIGALRM");
    }
    if (signal(SIGSEGV, sig_alrm_handler) == SIG_ERR) {
        err_ret("Can't catch SIGSEGV");
    }
    if (signal(SIGFPE, sig_alrm_handler) == SIG_ERR) {
        err_ret("Can't catch SIGFPE");
    }
    // 不可以捕获 SIGINT(Ctrl+C), 程序在 sleep() 阻塞,按下按键,执行 signal_handler 之后
    // 程序回不去 sleep() 之前的上下文了, 所以一直触发 SIGFPE 异常, 执行信号回调
    // if (signal(SIGINT, sig_alrm_handler) == SIG_ERR) {
    //     err_ret("Can't catch SIGINT");
    // }
    for (;;) {
        if((ptr = getpwnam("root")) == NULL) {
            err_sys("error");
        }
        if (strcmp(ptr->pw_name, "root") != 0) {
            printf("return value corrupted! pw_name=%s\n", ptr->pw_name);
        } else {
            printf("correct\n");
        }
        sleep(5);
        printf("raise a SIGFPE fault\n");
        // printf("%d\n", 1/num++); // 1/0
    }
}
static void sig_alrm_handler(int signo)
{
    /* 信号的处理函数中不能调用不可重入函数 , eg. malloc(), free(), printf()... */
    // getpwnam() 中使用了 static 变量, 是不可重入函数
    if (signo == SIGALRM) {
        struct passwd *rootptr;
        if ((rootptr = getpwnam("root")) == NULL) {
            err_sys("get present work name");
        }
        alarm(1);
    } else if (signo == SIGSEGV) {
        printf("SIGSEGV occur\n");
    } else if (signo == SIGFPE) {
        printf("SIGFPE occur\n");
    } else if (signo == SIGINT) {
        printf("SIGINT occur, do something\n");
    } else {
    }
}

static void sig_child_handler(int signo);
void sig_child_test(void)
{
    pid_t pid;

    if (signal(SIGCLD, sig_child_handler) == SIG_ERR) {
        perror("signal error");
    }

    if ((pid = fork()) < 0) {
        perror("fork error");
    } else if (pid == 0) { // child
        printf("child:%d %d running\n", getpid(), getppid());
        sleep(2);
        printf("child exit\n");
        _exit(0);
    } else { // parent
        printf("parent:%d %d running\n", getpid(), getppid());
    }
    pause(); // 阻塞等待, 直到有信号到来, 唤醒父进程
    printf("parent wake up\n");
    sleep(2);
    printf("parent exit\n");
    exit(0);
}
static void sig_child_handler(int signo)
{
    // 在信号处理函数中等待? 软件中断中!!! 应该不可行, 仅仅为了测试
    pid_t pid;
    int status;

    printf("SIGCLD received\n"); // printf() 也不可以调用
    if (signal(SIGCLD, sig_child_handler) == SIG_ERR) // 重新注册信号
        perror("signal error");
    if ((pid = wait(&status)) < 0) {
        perror("wait error");
    }
    printf("exit process pid = %d\n", pid);
}

/* 每个进程只能有一个闹钟时间 alarm */
static void alarm_sig_handler2(int signo) {}
unsigned int sleep1(unsigned int seconds);
unsigned int sleep2(unsigned int seconds);
void sig_test4(void)
{
    printf("Enter\n");
    // sleep1(2);
    sleep2(2);
    printf("exit\n");
}

/**
 * @fun: 自己实现一个 sleep() 使得进程休眠一段时间\
 * @desc: 这个 sleep1 存在三个问题: 如果 alarm 在 sleep1() 之前有使用; 竞争条件...
 * @param: 休眠时间
 * @ret: 返回没有休眠的时间
*/
unsigned int sleep1(unsigned int seconds)
{
    // 谁调用就在谁的进程注册 SIGALRM 信号
    if (signal(SIGALRM, alarm_sig_handler2) == SIG_ERR) {
        return seconds;
    }
    alarm(seconds); // 在一个高速运行系统中, SIGALRM 的信号处理程序可能比 pause 先运行, 导致进程一直在 pause()
    pause();
    return alarm(0);
}

static jmp_buf env_alarm;
static void alarm_sig_handler3(int signo)
{
    longjmp(env_alarm, 1);
}

/**
 * @desc: 为了解决竞争条件
 *        问题:SIGALRM 会中断其他信号处理程序
*/
unsigned int sleep2(unsigned int seconds)
{
    if (signal(SIGALRM, alarm_sig_handler3) == SIG_ERR) {
        return seconds;
    }
    if (setjmp(env_alarm) == 0) {
        alarm(seconds);
        pause();
    }
    return alarm(0);
}

static void sig_int1(int signo);
int sig_test5(void)
{
    unsigned int unslept;
    if (signal(SIGINT, sig_int1) == SIG_ERR) {
        err_sys("register signal failed");
    }
    // pause();

    // longjmp() 如果打断之前的信号处理, 信号处理执行完毕后, 不会继续之前其他信号的信号处理程序
    unslept = sleep2(1); 
    printf("sleep return:%u\n", unslept);
    exit(0);
}
static void sig_int1(int signo)
{
    int i, j;
    volatile int k; // 防止被编译器优化

    // 延时一段时间, 大约 2s
    printf("sig_int start\n");
    for (i = 0; i < 300000; i++)
        for (j = 0; j < 4000; ++j)
            k += i * j;
    printf("sig_int exit\n");
}

static void alarm_sig_handler4(int signo);
// alarm() 还可以用来中断对一些低速设备的访问, 一段时间后停止执行该操作
void sig_alarm_test1(void)
{
    int n;
    char line[MAXLINE];

    if (signal(SIGALRM, alarm_sig_handler4) == SIG_ERR) {
        err_sys("error");
    }
    alarm(5);
    /* 运行正常 难道是 Linux signal() 注册的信号, 被中断的系统调用默认是继续执行的? */
    if ((n = read(STDIN_FILENO, line, MAXLINE)) < 0) {
        err_ret("read error");
    }
    alarm(0); // 取消闹钟
    line[n] = '\n';
    if (write(STDOUT_FILENO, line, n) != n) {
        err_ret("write failed");
    }
    exit(0);
}
static void alarm_sig_handler4(int signo)
{
    // 什么都不干, 仅仅中断阻塞的调用
    printf("%s\n", __func__);
}

/* longjmp() 可以确保低速的系统调用不被信号中断 */
static void alarm_sig_handler5(int signo);
void sig_test6(void)
{
    int n;
    char line[MAXLINE];
    unsigned int unslept;

    if (signal(SIGALRM, alarm_sig_handler5) == SIG_ERR)
        err_sys("register signal failed");
    
    if (setjmp(env_alarm) != 0) {
        err_sys("read timeout");
    }
    printf("prepare to read\n");
    /* alarm 为什么只会运行一次??? */
    unslept = alarm(2);
    printf("unslept:%u\n", unslept);
    if ((n = read(STDIN_FILENO, line, MAXLINE)) < 0) {
        err_sys("read err");
    }
    alarm(0);
    if (write(STDOUT_FILENO, line, n) != n) {
        err_sys("write err");
    }
    exit(0);
}
static void alarm_sig_handler5(int signo)
{
    printf("%s\n", __func__);
    longjmp(env_alarm, 1);
}


// 自己通过位的方式实现 sigset_t 类型
#define sigemptyset_m(ptr) (*ptr = 0)
#define sigfillset_m(ptr)  (*(ptr) = ~(sigset_t_m)0, 0) /* 逗号表达式, 最后的是返回值 */
// #define sigfillset_m(ptr)           \
//     do {                            \
//         *(ptr) = ~((sigset_t_m)0);  \
//         return 0;                   \
//     } while(0)
#define SIGBAD_M(signo)    ((signo) <= 0 || (signo) >= NSIG_M)
#define NSIG_M              32
typedef int sigset_t_m; // 总共有 32 种信号,每一个位表示一个信号
int sigaddset_m(sigset_t_m *set, int signo)
{
    if (SIGBAD_M(signo)) {
        errno = EINVAL;
        return -1;
    }
    *set |= 1 << (signo - 1); // 置1
    return 0;
}

int sigdelset_m(sigset_t_m *set, int signo)
{
    if (SIGBAD_M(signo)) {
        errno = EINVAL;
        return -1;
    }
    *set &= ~(1 << (signo - 1)); // 置0
    return 0;
}

int sigismember_m(sigset_t_m *set, int signo)
{
    if (SIGBAD_M(signo)) {
        errno = EINVAL;
        return -1;
    }
    return ((*set & (1 << (signo - 1))) != 0);
}
// 只支持 int 类型二进制
static void itoa_m(int num, char *out, int jinzhi)
{
    for (int ix = 31, jx = 0; ix >= 0; --ix, ++jx) {
        if ((num >> ix) & 0x01) {
            out[jx] = '1';
        } else {
            out[jx] = '0';
        }
    }
}
static void print_sig(sigset_t_m *ptr)
{
    char str[64];
    itoa_m(*ptr, str, 2);
    // itoa并不是一个标准的C函数，它是Windows特有的，如果要写跨平台的程序，请用sprintf。
    printf("value:%s\n", str);
}

void my_sig_test(void)
{
    sigset_t_m sig;
    sigemptyset_m(&sig);
    print_sig(&sig);
    // sigfillset_m(&sig);
    // print_sig(&sig);
    sigaddset_m(&sig, SIGINT);
    if (sigismember_m(&sig, SIGINT)) {
        printf("bit:%d is set\n", SIGINT);
    } else {
        printf("bit:%d is not set\n", SIGINT);
    }
    print_sig(&sig);
    sigdelset_m(&sig, SIGINT);
    print_sig(&sig);
}

static void sig_quit_handler(int signo);
void sig_proc_mask_test1(void)
{
    char pid[32];
    sigset_t newmask, oldmask, pendmask;

    if (signal(SIGQUIT, sig_quit_handler) == SIG_ERR) {
        err_ret("can't catch SIGQUIT");
    }

    // Block SIGQUIT
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGQUIT);
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        err_sys("SIG_BLOCK ERR");
    }
    printf("SIGQUIT BLOCKED\n");


    sleep(5); // SIG_QUIT is pending

    if (sigpending(&pendmask) < 0) {
        err_sys("get pending signal failed");
    }
    if (sigismember(&pendmask, SIGQUIT)) {
        printf("SIGQUIT pending\n");
    } else {
        printf("SIGQUIT is not pending\n");
    }

    // restore signal mask
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        err_sys("SIG_SETMASK ERR");
    }
    printf("SIGQUIT UNBLOCKED\n");
    sleep(5);
    exit(0);
    // signal(SIGFPE, SIG_IGN); // 必须要使用 sigprocmask() 设置 mask 才有效
    // signal(SIGALRM, SIG_IGN);
    // snprintf(pid, sizeof(pid), "pid:%d", getpid());
    // pr_mask(pid);
}

static void sig_quit_handler(int signo)
{
    printf("Caught SIGQUIT\n");
    if (signal(SIGQUIT, SIG_DFL) == SIG_ERR) {
        err_sys("Can't reset SIGQUIT");
    }
}
// void sig_proc_mask_test2()

//使用 sigaction() 实现 signal()
Sigfunc *signal_m(int signo, Sigfunc *func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    } else {
        act.sa_flags |= SA_RESTART; // handler 结束后重启被中断的系统调用
    }
    if (sigaction(signo, &act, &oact) < 0) {
        return (SIG_ERR);
    }

    return (oact.sa_handler); // 返回的是之前的 signal_handler
}

Sigfunc *signal_intr(int signo, Sigfunc *func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT; // handler 结束后 不重启 被中断的系统调用
#endif
    if (sigaction(signo, &act, &oact) < 0) {
        return (SIG_ERR);
    }

    return (oact.sa_handler);
}

static void fpe_signal_handler(int signo);

void sig_test7()
{
    int num = 0;
    int ret;
    int cnt = 0;
    if (signal_m(SIGFPE, fpe_signal_handler) == SIG_ERR) {
    // if (signal_intr(SIGFPE, fpe_signal_handler) == SIG_ERR) {
        err_ret("catch SIGFPE failed");
    }

    for (;;) {
        printf("result:\n");
        if (cnt++ == 0) // 只执行一次
            // ret = 1 / num; // 为什么会一直触发 SIGFPE 这个信号?????????
            ;
        sleep(5);
    }
}

static void fpe_signal_handler(int signo)
{
    printf("catch SIGFPE\n");
}

static void sig_usr1(int signo);
static void sig_alrm(int);
static sigjmp_buf jmpbuf;
static volatile sig_atomic_t canjump; // 可以跳转标志位
void sig_setjmp_test2(void)
{
    if (signal(SIGUSR1, sig_usr1) == SIG_ERR) {
        err_sys("SIGUSR1 err");
    }
    if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
        err_sys("SIGALRM err");
    }
    pr_mask("start main:");

    if (sigsetjmp(jmpbuf, 1)) { // sigsetjmp() 成功返回 0; 从 siglongjmp() 返回非零
        pr_mask("ending main:");
        exit(0);
    }

    canjump = 1; //
    for (;;)
        pause();
}

static void sig_usr1(int signo)
{
    time_t starttime;

    if (canjump == 0)
        return;
    pr_mask("starting sig_usr1:"); // 在执行当前信号处理程序时,当前信号会自动被屏蔽
    alarm(3);
    starttime = time(NULL);
    for (;;) { // delay 5s
        if (time(NULL) > starttime + 5)
            break;
    }
    pr_mask("finishing sig_usr1:");
    canjump = 0;
    siglongjmp(jmpbuf, 1); // 跳转到 main 函数, 而不是直接 return
}

static void sig_alrm(int signo)
{
    pr_mask("in sig_alrm:"); // 在执行当前信号处理程序时,当前信号会自动被屏蔽
}

static void int_signal_handler(int signo);
void sig_suspend_test1(void)
{
    sigset_t newmask, oldmask, waitmask;

    pr_mask("program start");
    if (signal(SIGINT, int_signal_handler) == SIG_ERR) {
        err_ret("register SIGINT failed");
    }

    sigemptyset(&waitmask);
    sigaddset(&waitmask, SIGUSR1);

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);

    // block SIGINT
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        err_ret("SIG_BLOCK failed");
    }

    // 临界段代码开始
    pr_mask("Critical region");
    printf("This is criticcal region code\n");
    sleep(1);
    // 临界段代码结束

    // pause, allow all signals except SIGUSR1
    // 这里阻塞等待除了 SIGUSR1 之外的其他所有信号, 等待成功返回 -1
    if (sigsuspend(&waitmask) != -1) {
        err_ret("sig suspend failed");
    }
    pr_mask("after return from sigsuspend");

    // reset signals before block
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        err_ret("SIG_SETMASK failed");
    }
    pr_mask("program exit");
    exit(0);

}
static void int_signal_handler(int signo)
{
    pr_mask("int handler");
}

static int quitflag = 0;
static void int_signal_handler2(int signo);

void sig_suspend_test2(void)
{
    sigset_t newmask, oldmask, zero_mask;

    if (signal(SIGINT, int_signal_handler2) == SIG_ERR) {
        err_ret("SIGINT failed");
    }
    if (signal(SIGQUIT, int_signal_handler2) == SIG_ERR) {
        err_ret("SIGQUIT failed");
    }

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGQUIT);
    sigemptyset(&zero_mask);
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask)) {
        err_sys("SIGBLOCK ERR");
    }

    while (quitflag == 0)
        sigsuspend(&zero_mask); // 阻塞等待任何信号,但是只有 SIGQUIT 可以让他退出循环, SIGSTOP 呢?
    
    // reset signals
    pr_mask("before quit");
    quitflag = 0;
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        err_ret("SIG_SETMASK error");
    }
    pr_mask("after quit");

    exit(0);
}

static void int_signal_handler2(int signo)
{
    if (signo == SIGINT)
        printf("interrupt\n");
    else if (signo == SIGQUIT)
        quitflag = 1;
    pr_mask("signal handler");
}

// 通过信号实现父子进程同步, SIGUSR1 由父进程发送给子进程, SIGUSR2 由子进程发送到父进程
// TELL_WAIT, TELL_PARENT, TELL_CHILD, WAIT_PARENT, WAIT_CHILD
static volatile sig_atomic_t sigflag;
static sigset_t newmask, oldmask, zeromask;
static void usr_signal_handler(int signo)
{
    sigflag = 1;
}

void TELL_WAIT(void)
{
    if (signal(SIGUSR1, usr_signal_handler) == SIG_ERR)
        err_ret("SIGUSR1 err");
    if (signal(SIGUSR2, usr_signal_handler) == SIG_ERR)
        err_ret("SIGUSR2 err");

    sigemptyset(&zeromask);
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    sigaddset(&newmask, SIGUSR2);
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        printf("SIG_BLOCK failed\n");
    }
}

// tell parent we're done
void TELL_PARENT(pid_t pid)
{
    kill(pid, SIGUSR2);
}

// wait for parent
void WAIT_PARENT(void)
{
    while (sigflag == 0)
        sigsuspend(&zeromask);
    // reset signals
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        err_ret("SIG_SETMASK failed");
    }
}

// tell child we're done
void TELL_CHILD(pid_t pid)
{
    kill(pid, SIGUSR1);
}

// wait for child
void WAIT_CHILD(void)
{
    while (sigflag == 0)
        sigsuspend(&zeromask);
    sigflag = 0;
    // reset signal mask
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        err_ret("SIG_SETMASK failed");
    }
}

// abort() 终止进程, 对打开的 I/O流关闭
// abort() 可能的内部实现
void abort1(void)
{
    sigset_t mask;
    struct sigaction act;

    sigaction(SIGABRT, NULL, &act);
    if (act.sa_handler == SIG_IGN) { // 之前的行为是忽略
        act.sa_handler = SIG_DFL;
        sigaction(SIGABRT, &act, NULL);
    }
    if (act.sa_handler == SIG_DFL) {
        fflush(NULL); // flush all open stdio streams
    }

    sigfillset(&mask);
    sigdelset(&mask, SIGABRT);
    sigprocmask(SIG_SETMASK, &mask, NULL); // 屏蔽除 SIGABRT 之外的所有信号
    kill(getpid(), SIGABRT); // 给自己发信号

    // 程序已经捕获到了 SIGABRT 并且执行完了信号处理程序, 按道理程序应该退出了
    fflush(NULL);
    act.sa_handler = SIG_DFL;
    sigaction(SIGABRT, &act, NULL);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    kill(getpid(), SIGABRT);
    exit(1); // 程序不应该运行到这里, 因为 SIGABRT 的默认行为就是退出程序了
}

void abort_test1(void)
{
    printf("start abort test\n");
    abort1();
    printf("end abort test\n");
}

// system test
static void int_signal_handler5(int signo);
static void child_signal_handler5(int signo);
int system_m1(const char *cmdstring);
void system_test1(void)
{
    if (signal(SIGINT, int_signal_handler5) == SIG_ERR) {
        err_ret("signal int err");
    }
    if (signal(SIGCHLD, child_signal_handler5) == SIG_ERR) {
        err_ret("signal child err");
    }
    if (system("/bin/ed") < 0) {
    // if (system_m1("/bin/ed") < 0) {
        err_ret("system err");
    }
    exit(0);
}
static void int_signal_handler5(int signo)
{
    printf("CAUGHT SIGINT\n");
}

static void child_signal_handler5(int signo)
{
    printf("CAUGHT SIGCHLD\n");
}

// 可能的 system() 实现
// POSIX.1 要求 system() 捕获:SIGINT, SIGQUIT, 忽略 SIGQUIT
int system_m1(const char *cmdstring)
{
    pid_t pid;
    int status;
    struct sigaction ignore, saveintr, savequit;
    sigset_t childmask, savemask;

    if (!cmdstring) {
        return 1;
    }

    // ignore SIGINT & SIGQUIT
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    ignore.sa_flags = 0;
    if (sigaction(SIGINT, &ignore, &saveintr) < 0) {
        return -1;
    }
    if (sigaction(SIGQUIT, &ignore, &savequit) < 0) {
        return -1;
    }

    // block SIGCHILD
    sigemptyset(&childmask);
    sigaddset(&childmask, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &childmask, &savemask) < 0) {
        printf("SIG_BLOCK failed\n");
        return -1;
    }

    if ((pid = fork()) < 0) {
        status = -1;
    } else if (pid == 0) { // child
        // restore signal actions & reset signal mask
        sigaction(SIGINT, &saveintr, NULL);
        sigaction(SIGQUIT, &savequit, NULL);
        sigprocmask(SIG_SETMASK, &savemask, NULL);
        execl("/bin/sh", "sh", "-c", cmdstring, (char*)0);
        _exit(127); // int8_t:-128~127
    } else { // parent
        // 父进程等待子进程结束
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                status = -1; // error other than EINTR from waitpid
                break;
            }
        }
    }

    // reset signal actions & reset signal mask
    sigaction(SIGINT, &saveintr, NULL);
    sigaction(SIGQUIT, &savequit, NULL);
    sigprocmask(SIG_SETMASK, &savemask, NULL);

    return status;
}

static void test_signal_handler2(int signo, siginfo_t *info, void *param);
void signal_test9(void)
{
    siginfo_t info = {0};
    struct sigaction act;
    sigset_t zeromask2;
    char *p_tr;


    psignal(SIGINT, "signal_test9");
    printf("SIGKILL:%s\n", strsignal(SIGKILL));

    // psiginfo(&info, p_str); // siginfo_t
    // printf("siginfo:%s\n", p_str);
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = test_signal_handler2;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        err_ret("sigact failed");
    }
    printf("pause:\n");
    // for(;;)
    pause();

}

static void test_signal_handler2(int signo, siginfo_t *info, void *param)
{
    printf("CATCH signal:%d\n", signo);
    psiginfo(info, "test_signal_handler2");
}
