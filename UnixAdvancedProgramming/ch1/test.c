#include "test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> // calculate file operation time
#include <error.h> // errno
#include <limits.h> // INT_MAX

/*
 *	POSIX Standard: 5.6 File Characteristics	<sys/stat.h>
 */
#include <sys/stat.h>

/*
 *	POSIX Standard: 5.1.2 Directory Operations	<dirent.h>
 */
#include <dirent.h>

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/*
 *	POSIX Standard: 2.6 Primitive System Data Types	<sys/types.h>
 */
#include <sys/types.h>

// #include <sys/signal.h>
/*
 *	ISO C99 Standard: 7.14 Signal handling <signal.h>
 */
#include <signal.h>

/*
 *	POSIX Standard: 3.2.1 Wait for Process Termination	<sys/wait.h>
 */

#include <sys/wait.h>


/*************
 * MACRO 
 *************/
#define UNUSED_PARAM(p)       ((void)p)

#define err_quit(format, ...) printf(format", log output.\n", ##__VA_ARGS__)
#define err_sys(format, ...)  printf(format", log output.\n", ##__VA_ARGS__)
#define err_ret(format, ...)  printf(format", log output.\n", ##__VA_ARGS__)

#define BUFFERSIZE            4096
#define MAXLINE               1028

void print_hello(void *param)
{
    UNUSED_PARAM(param);
    printf("%s:%d %s", __FILE__, __LINE__, __func__);
    return;
}

/**
 * @fun: file operation function test
 *       // 
     
*/
void file_operation_test()
{
    // file system operation function
    // stat()
    const char *path = "/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin";
    FILE *fp = fopen(path, "rw");
    char arr[10] = {1, 2, 3};

    if(!fp) {
        printf("open %s failed\r\n", path);
        return;
    }

    fprintf(fp, "%s", __FILE__);
    fwrite(arr, sizeof(char), sizeof(arr), fp);
    printf("write len:%ld\r\n", sizeof(arr));
    fclose(fp);
    fp = NULL;

    return;
}

/**
 * @fun: stat system call
 *       int fstat(int filedes, struct stat *buf);
 *       int stat(const char *path, struct stat *buf);
 *       int lstat(const char *path, struct stat *buf);
 *       difference:
 *       fstat() need open file to get the file pointer filedes; 
*/
void file_stat_test()
{
    struct stat st = {0};
    char *pArr = NULL;
    FILE *fp = NULL;
    const char *path = "/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin";
    clock_t clkStart, clkEnd;

    stat("/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin", &st);
    printf("%s size:%ld\r\n", path, st.st_size);
    pArr = (char*)malloc(st.st_size);

    // does not check return value temporarily
    fp = fopen("/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin", "rb");
    clkStart = clock();
    fread(pArr, sizeof(char), st.st_size, fp);
    clkEnd = clock();
    printf("read test consume time:%lu\r\n", clkEnd - clkStart);
    fclose(fp);

    fp = fopen("/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin", "rb");
    clkStart = clock();
    fwrite(pArr, sizeof(char), st.st_size, fp);
    clkEnd = clock();
    printf("read test consume time:%lu\r\n", clkEnd - clkStart);
    fclose(fp);

    return;
}


/**
 * @fun: list file int the directory
 * 
 * @param
 * @param
 * @ret:0-success; 1-255:error occur
*/
int directory_op_test(int argc, char **argv)
{
    DIR *dp;
    struct dirent *dirp;

    if(argc != 2) {
        err_quit("usage: ls directory_name\r\n");
        return 1;
    }

    if((dp = opendir(argv[1])) == NULL) {
        err_sys("Can't open %s\r\n", argv[1]);
        return 2;
    }

    while((dirp = readdir(dp)) != NULL) {
        printf("%s\r\n", dirp->d_name);
    }

    closedir(dp);

    // program normally exit
    exit(0);

    return 0;
}

/**
 * @fun:将标准输入拷贝到标准输出
 *      可以用于复制任意 UNIX 普通文件
 *      CTRL + D 退出
*/
int input_output_test()
{
    int n;
    char buf[BUFFERSIZE];

    // 每次运行一个程序, 所有的 shell 都会为其打开三个文件表述符:stdin, stdout, stderr
    // 所以不需要打开文件可以直接操作
    while(n = read(STDIN_FILENO, buf, 10)) { // 为什么会阻塞在这里????????????????????
        printf("read len:%d\r\n", n);
        if(write(STDOUT_FILENO, buf, n) != n) {
            err_sys("write error\r\n");
        }
        printf("write len:%d\r\n", n);
    }
    if(n < 0) {
        err_sys("read error\r\n");
    }

    exit(0);

    return 0;
}

int input_output_test2()
{
    int c;

    while((c = getc(stdin)) != EOF) {
        printf("read:%c\r\n", c);
        if(putc(c, stdout) == EOF) {
            err_sys("write error\r\n");
        }
        printf("write:%c\r\n", c);
    }

    exit(0);

    return 0;
}

/**
 * @fun: UNIX 的进程控制功能
 *       fork() 创建一个新进程
 *       exec() 执行某一个可执行程序
 *       waitpid() 父进程等待子进程, 返回子进程的结束状态, 可以通过这个返回值获取到子进程的结束原因
*/
void process_test(void)
{
    char buf[MAXLINE];
    pid_t pid;
    int status;

    printf("%%");
    while(fgets(buf, MAXLINE, stdin) != NULL) {
        if(buf[strlen(buf) - 1] == '\n') { // replace last character \n with \0
            buf[strlen(buf) - 1] = 0;
        }

        // fork() 函数返回两次:
        // 1) 父进程返回 PID;
        // 2) 子进程返回 0;
        if((pid = fork()) < 0) { // create process failed
            err_sys("fork error\r\n");
        }
        else if(pid == 0) { // child process return
            printf("child process, process ID:%ld\r\n", (long)pid);
            execlp(buf, buf, (char*)0); // execute received command
            err_ret("could't execute:%s\r\n", buf);
            exit(127);
        }
        else { // parent process
            printf("parent process, chiil process ID:%ld\r\n", (long)pid);
        }

        // parent process wait child process end
        if((pid = waitpid(pid, &status, 0)) < 0) {
            err_sys("waitpid error\r\n");
        }
        printf("%%");
    }

    exit(0); // normally exit
}

/**
 * @fun:对于多线程的环境, 每个线程都有一个自己的 errno, 以避免线程之间干扰
 *      Linux 中的操作:
 *      #define int *__errno_location(void);
 *      #define errno (*__errno_location())
 *      系统可通过出错类型来判断出现的错误是否是可恢复的; 可恢复的, 延时然后重试; 不可恢复...
*/
void err_test(int argc, char *argv[])
{
    // fprintf(stderr, "EACCES:%s\n", strerror(SACCES));
    // errno = ENOENT;
    // perror(argv[0]);
    // exit(0);
}

void uid_gid_test()
{
    printf("uid = %d, gid = %d\r\n", getuid(), getgid());

}

/**
 * @fun: signal
 *       信号用来向进程发送信号 
 *       (1) 按键 Ctrl + D;
 *       (2) kill() 函数;
*/
// our singnal-catching function
static void sig_int(int param)
{
    printf("%s get a SIGINT\r\n", __func__);
    return;
}

void signal_test()
{
    char buf[MAXLINE] = {0};
    pid_t pid;
    int status;
    clock_t startClock, endClock;

    if(signal(SIGINT, sig_int) == SIG_ERR) { // 创建一个捕获 SIGINT 的信号
        err_sys("signal error");
    }

    printf("%s\n", __func__);

    while(fgets(buf, MAXLINE, stdin) != NULL) { // 这里会阻塞
        if(buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
        
        if((pid = fork()) < 0) {
            err_sys("fork error");
        }
        else if(pid == 0) {
            startClock = clock();
            execlp(buf, buf, (char*)0);
            err_sys("couldn't execute %s", buf);
        }
        else {
            printf("child process create success, pid:%ld\r\n", (long)pid);
        }

        // parent
        if((pid = waitpid(pid, &status, 0)) < 0) {
            err_sys("wait pid err");
        }
        endClock = clock();
        printf("child process consume:%lu\r\n", endClock - startClock);
        printf("%%");
    }

    exit(0);

    return;
}

/**
 * 
*/
void time_test(void)
{

}