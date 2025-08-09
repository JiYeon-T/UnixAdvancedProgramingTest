#include "../common/apue.h"
#include <sys/wait.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ipc.h>
/***************************************************************
 * Macro
 ***************************************************************/
#define DEF_PAGER       "/bin/more" // default pager program
#define PAGER          "${PAGER:-more}" // 如果 shell 变量 PAGER 没有定义就是用 more
/***************************************************************
 * Function Declaration
 ***************************************************************/
int pipe(int fd[2]);
FILE *popen(const char *cmdstring, const char *type);
int pclose(FILE *fp);

// #include <stdio.h> check and reset stream status
extern void clearerr(FILE *stream);
extern int feof(FILE *stream);
extern int ferror(FILE *stream);
extern int fileno(FILE *stream);

extern int mkfifo(const char *path, mode_t mode);
extern int mkdifoat(int fd, const char *path, mode_t mode);

key_t ftok(const char *path, int id);

// 命令行输入参数处理 api
extern int getopt(int argc, char * const argv[], const char * options);
extern int optind, otperr, optopt; // 与 getopt() 有关的 4 个全局变量
extern char *optarg;
/***************************************************************
 * Private Function Declaration
 ***************************************************************/
static void sig_pipe_handler(int signo);
/***************************************************************
 * Private Variables
 ***************************************************************/
static int pfd1[2], pfd2[2];
static pid_t *childpid = NULL; // popen() 实现
static int maxfd;
/***************************************************************
 * Public Variables
 ***************************************************************/

// 创建一个父进程到子进程的 pipe
void pipe_test1(void)
{
    int n;
    int fd[2];
    pid_t pid;
    char line[BUFFERSIZE];

    if (pipe(fd) < 0)
        err_ret("pipe err")
    
    if ((pid = fork()) < 0) {
        err_ret("fork failed\n");
    } else if (pid > 0) { // parent
        close(fd[0]);
        write(fd[1], "abcde", 6);
        printf("%d:%d send message\n", getpid(), getppid());
    } else { // child
        // sleep(1); // WAIT_PARENT send
        close(fd[1]);
        n = read(fd[0], line, BUFFERSIZE);
        printf("%d:%d recv message:%s len:%d\n", getpid(), getppid(), line, n);
        // write(STDOUT_FILENO, line, n);
    }

    // sleep(2);
    // exit(0);
    return;
}

// 创建一个子线程以及管道, 管道的读端作为子线程的标准输入,
void pipe_test2(int argc, char *argv[])
{
    int n;
    int fd[2];
    pid_t pid;
    char *pager, *argv0;
    char line[BUFFERSIZE];
    FILE *fp;

    if (argc != 2)
        err_ret("usage:a.out <path.name>");

    if ((fp = fopen(argv[1], "r")) == NULL)
        err_sys("can't open file:%s", argv[1]);
    if (pipe(fd) < 0)
        err_ret("create pile failed");
    
    if ((pid = fork()) < 0) {
        err_ret("create process failed");
    } else if (pid > 0) { // parent
        close(fd[0]); // close read end
        // parent copy files to pipe
        while (fgets(line, BUFFERSIZE, fp) != NULL) {
            n = strlen(line);
            printf("read len:%d\n", n);
            if (write(fd[1], line, n) != n)
                err_sys("pipe write failed");
        }
        if (ferror(fp))
            err_sys("fgets err");
        close(fd[1]); // 关闭管道写端
        fprintf(stdout, "wait child exit\n");
        // 等待子进程结束语
        if (waitpid(pid, NULL, 0) < 0)
            err_sys("waitpid failed");
    } else { // child
        close(fd[1]); // close write end
        // 管道的输入作为子进程的标准输入
        if (fd[0] != STDIN_FILENO) {
            if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO)
                err_sys("dup2 failed");
            // don't need this after dup
            close(fd[0]);

            // get a pager to execute this
            if ((pager = getenv("PAGER")) == NULL)
                pager = DEF_PAGER;
            if ((argv0 = strrchr(pager, '/')) != NULL)
                argv0++;
            else
                argv0 = pager;

            if (execl(pager, argv0, (char*)0) < 0)
                err_sys("excel error for %s", pager);
        }
    }
    exit(0);
}

// 通过管道实现父子进程间同步
void TELL_WAIT(void)
{
    if (pipe(pfd1) < 0 || pipe(pfd2) < 0)
        err_sys("pipe err");
}

void TELL_PARENT(pid_t pid)
{
    if (write(pfd2[1], "c", 1) != 1)
        err_sys("write err");
}

void WAIT_PARENT(void)
{
    char c;

    if (read(pfd1[0], &c, 1) != 1)
        err_sys("read err");

    if (c != 'p')
        err_quit("WAIT_PARERNT incorrect data:%c", c);
}

void TELL_CHILD(pid_t pid)
{
    if (write(pfd1[1], "p", 1) != 1)
        err_sys("write err");
}

void WAIT_CHILD(void)
{
    char c;

    if (read(pfd2[0], &c, 1) != 1)
        err_sys("read err");
    if (c != 'c')
        err_quit("WAIT_CHILD recv incorrect data:%c", c);
}

void parent_child_sync_test1(void)
{
    pid_t pid;
    TELL_WAIT();

    printf("main thread running\n");
    if ((pid = fork()) < 0) {
        err_ret("fork failed");
    } else if (pid == 0) { // PARENT
        printf("parent init pid:%d ppid:%d\n", getpid(), getppid()); // 1.父进程初始化完成, 等待子进程运行
        WAIT_CHILD(); // 1. 
        TELL_CHILD(pid); // 让子进程开始运行
        printf("parent running\n");
        sleep(2);
        
        TELL_CHILD(pid); // 让子进程停止

        sleep(5);
        printf("parent exit\n");
        exit(0);
    } else { // child
        printf("child init pid:%d ppid:%d\n", getpid(), getppid()); // 2.子进程初始化完成, 通知父进程运行
        TELL_PARENT(pid);
        WAIT_PARENT(); // 等待父进程再次让子进程运行
        printf("child running...\n");
        sleep(2);
        WAIT_PARENT(); // 等待父进程让子进程终止
        printf("child end\n");
        exit(0);
    }
    printf("main process exit abnormal\n");
}

// 使用 popen 重新实现 PAGER 阅读
void pager_test2(int argc, char *argv[])
{
    char line[BUFFERSIZE];
    FILE *fpin, *fpout;

    if (argc != 2)
        err_ret("usage: a.out <file_name>");
    
    if ((fpin = fopen(argv[1], "r")) == NULL)
        err_ret("file:%s open failed", argv[1]);
    if ((fpout = popen(PAGER, "w")) == NULL)
        err_ret("cmdstring:%s open failed", PAGER);
    
    // copy file data to cmdstring pile
    while(fgets(line, BUFFERSIZE, fpin) != NULL) {
        printf("read len:%lu\n", strlen(line));
        if (fputs(line, fpout) == EOF)
            err_sys("fputs failed");
    }

    if (ferror(fpin) != 0)
        err_ret("fpin err");
    
    printf("pclose exit1\n");
    if (pclose(fpout) == -1) // 关闭标准 IO 流, 等待命令终止, 返回 shell 的终止状态
        err_sys("pclose err");
    printf("pclose exit2\n");
    fclose(fpin);

    exit(0);
}

// 自己实现 popen() 和 pclose()
FILE *popen_m(const char *cmdstring, const char *type)
{
    int i;
    int pfd[2];
    pid_t pid;
    FILE *fp;

    if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) {
        errno = EINVAL;
        return (NULL);
    }

    if (childpid == NULL) {
        // TODO:
        // 可以优化的地方就, 使用链表, 而不是数组
        maxfd = 4096; //open_max();, sysconf() 获取最大值?
        if ((childpid = calloc(maxfd, sizeof(pid_t))) == NULL)
            return (NULL);
    }

    if (pipe(pfd) < 0)
        return (NULL); // errno set by pipe()
    if (pfd[0] >= maxfd || pfd[1] >= maxfd) {
        close(pfd[0]);
        close(pfd[1]);
        errno = EMFILE;
        return (NULL);
    }

    if ((pid = fork()) < 0)
        return (NULL); // errno set by fork()
    else if (pid == 0) { // child
        if (type[0] == 'r') {
            close(pfd[0]);
            if (pfd[1] != STDOUT_FILENO) {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);
            }
        } else { // 'w'
            close(pfd[1]);
            if (pfd[0] != STDIN_FILENO) {
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
            }
        }
        // close all descriptor in childpid[]
        for (i = 0; i < maxfd; ++i)
            if (childpid[i] > 0)
                close(i); // 保存子进程 pid 是使用文件描述符作为下标
        
        //
        execl("/bin/sh", "sh", "-c", cmdstring, (char*)0);
        _exit(127); // 子进程执行完命令后退出
    } else { // parent
    }

    // 父进程继续
    if (type[0] == 'r') {
        close(pfd[1]); // 父进程关闭写端
        if ((fp = fdopen(pfd[0], type)) == NULL) // 将已存在的文件描述符关联到另一个文件描述符
            return (NULL);
    } else { // ’w‘
        close(pfd[0]);
        if ((fp = fdopen(pfd[1], type)) == NULL)
            return (NULL);
    }

    // 在 childpid 中保存 pid, 文件描述符作为下标
    childpid[fileno(fp)] = pid;

    return (fp);
}

int pclose_m(FILE *fp)
{
    int fd, stat;
    pid_t pid;

    if (childpid == NULL) {
        errno = EINVAL;
        return (-1); // popen() has never been called
    }

    fd = fileno(fp);
    if (fd >= maxfd) {
        errno = EINVAL;
        return (-1);
    }

    if ((pid = childpid[fd]) == 0) {
        errno = EINVAL;
        return (-1);
    }

    childpid[fd] = 0;
    if (fclose(fp) == EOF)
        return (-1);
    
    // 等待子进程终止, 返回终止状态
    while (waitpid(pid, &stat, 0) < 0)
        if (errno != EINTR)
            return (-1);
    
    return (stat);
}

void popen_test1(void)
{
    char line[BUFFERSIZE];
    FILE *fpin;

    if ((fpin = popen("./myuclc", "r")) == NULL)
        err_sys("popen error");
    
    for (;;) {
        fputs("prompt> ", stdout);
        fflush(stdout); // 默认是行缓冲的
        if (fgets(line, BUFFERSIZE, fpin) == NULL) // 从管道读
            break;
        if (fputs(line, stdout) == EOF) // 输出
            err_sys("fputs error to pipe");
    }

    if (pclose(fpin) == -1)
        err_sys("pclose error");
    exit(0);
}

// 从标准输入读取数据, 向标准输出写数据, 和主线程通过管道连接
void coprocess_test1(void)
{
    int n, pfd1[2], pfd2[2]; // pfd1 父进程到子进程, pfd2 子进程到父进程
    pid_t pid;
    char line[BUFFERSIZE];

    if (signal(SIGPIPE, sig_pipe_handler) == SIG_ERR)
        err_ret("signal failed");
    
    if (pipe(pfd1) < 0 || pipe(pfd2) < 0)
        err_sys("pipe err");
    
    if ((pid = fork()) < 0) {
        err_ret("fork failed");
    } else if (pid > 0) { // parent
        close(pfd1[0]);
        close(pfd2[1]);
        while (fgets(line, BUFFERSIZE, stdin) != NULL) {
            n = strlen(line);
            if (write(pfd1[1], line, n) != n) // 发送到子进程
                err_ret("write failed");
            if ((n = read(pfd2[0], line, BUFFERSIZE)) < 0) // 从子进程获取结果
                err_ret("read error");
            if (n == 0) {
                err_ret("child close pipe");
                break;
            }
            line[n] = '\0';
            if (fputs(line, stdout) == EOF) // 结果输出
                err_sys("fputs error");
        }
        if (ferror(stdin) != 0)
            err_sys("fgets error");
        exit(0);
    } else { // child
        close(pfd1[1]);
        close(pfd2[0]);
        // 管道 pfd1 的读端作为子线程的标准输入
        if (pfd1[0] != STDIN_FILENO) {
            if (dup2(pfd1[0], STDIN_FILENO) != STDIN_FILENO)
                err_sys("dup2 error");
            close(pfd1[0]);
        }
        // 管道 pfd2 的写端作为子线程成的标准输出
        if (pfd2[1] != STDOUT_FILENO) {
            if (dup2(pfd2[1], STDOUT_FILENO) != STDOUT_FILENO)
                err_sys("dup2 error");
            close(pfd2[1]);
        }
        // 执行
        if (execl("./add2", "add2", (char*)0) < 0)
            err_sys("execl failed");
    }
    exit(0);
}

static void sig_pipe_handler(int signo)
{
    if (signo == SIGPIPE)
        printf("SIGPIPE caught\n");
    exit(0);
}

// TODO:
void getopt_test1(void)
{

}