/**
 * @fun 架构模型:
 * 客户端:发送路径(可以是设备/文件/套接字等等)以及读写权限
 * 服务器:返回文件描述符
 * 为什么客户端不自己直接打开呢? 反正在同一台电脑上.................
*/
#include <sys/uio.h>

#include "ch16.h"
#include  "../ch17_IPC/ch17.h"


/***************************************************************
 * Macro
 ***************************************************************/
#define BUFFSIZE        8192
#define MAXARGCNUM      50          // 客户端进程传入参数个数最大值
#define WHITE           "\t\n"      // white space for tokenizing argument
// argv 参数格式
//argv[0]_xxxxxx\0
//argv[1]_xxxxxx\0

char errmsg[MAXLINE];
int oflag;
char *pathname;
/***************************************************************
 * Private Function Declaration
 ***************************************************************/
static void string_termination_test(void);
static void strtok_test(void);
/***************************************************************
 * Public Function
 ***************************************************************/
int start_open_server_v1(int argc, char *argv[])
{
    int nread;
    char buf[MAXLINE];
    // char raw_buf[MAXLINE];
    UNUSED_PARAM(argc);
    UNUSED_PARAM(argv);

    // TODO:
    // 管道默认无缓冲, 字符串被单个解析了,
    // 没用, setvbuf 是针对标准库 API 的接口 
    // setvbuf(stdin, raw_buf, _IOLBF, MAXLINE);
    fprintf(stderr, "server start\n");
    for ( ; ; ) { // read arg buffer from client, process request
        if ((nread = read(STDIN_FILENO, buf, MAXLINE)) < 0) {
            err_sys("read error on stream pipe");
        }
        else if (nread == 0) // client has closed the stream pipe
            break;
        // TODO:
        // only for test
        // memset(buf, 0, MAXLINE);
        // strncpy(buf, "open test_io.txt 428", MAXLINE);
        // nread = strlen("open test_io.txt 428") + 1;
    
        fprintf(stderr, "server recv buf(%d):%s\n", nread, buf);
        // 管道默认无缓冲, 字符串被单个解析了
        handle_request(buf, nread, STDOUT_FILENO);
    }
}

/**
 * @fun 调用 buf_args() 将客户端请求分解成标准的 argv 型参数表,然后调用通 cli_args() 处理客户请求
 * @param[in] buf 输入内容
 * @param[in] nread length
 * @param[in] fd 结果发送到这个文件描述符
 * @ret None
*/
void handle_request(char *buf, int nread, int fd)
{
    int newfd;

    fprintf(stderr, "server recv(%d):%s\n", nread, buf);
    if (buf[nread - 1] != 0) {
        snprintf(errmsg, MAXLINE - 1, "buf not null terminated:%*.*s\n", nread, nread, buf);
        send_err(fd, -1, errmsg);
        return;
    }
    if (buf_args(buf, cli_args) < 0) {
        send_err(fd, -1, errmsg);
        return;
    }
    if ((newfd = open(pathname, oflag)) < 0) {
        snprintf(errmsg, MAXLINE - 1, "can't open %s:%s", pathname, strerror(errno));
        send_err(fd, -1, errmsg);
        return;
    }
    fprintf(stderr, "send_fd:%d\n", newfd);
    if (send_fd(fd, newfd) < 0)
        err_sys("send_fd error");
    close(newfd);

    return;
}

/**
 * @fun 将输入整理成 argv 格式
 * @param[in] buf
 * @param[in] optfunc
*/
int buf_args(char *buf, int (*optfunc)(int, char **))
{
    char *ptr, *argv[MAXARGCNUM];
    int argc;

    if (strtok(buf, WHITE) == NULL)
        return (-1);

    argc = 0;
    argv[argc] = buf;
    while ((ptr = strtok(NULL, WHITE)) != NULL) {
        if (++argc >= MAXARGCNUM - 1)
            return (-1);
        argv[++argc] = ptr;
    }

    // since argv pointers point into the user's buff
    // user's function can just copy pointers even though argv array will disappear on return

    return (optfunc(argc, argv));
}

static void string_termination_test(void)
{
    char buf[10] = {0, '  ',    '\0'};
    //              0x00, 0x20, 0x00
    for (int ix = 0; ix < sizeof(buf)/sizeof(buf[0]); ++ix) {
        printf("%#X\n", buf[ix]);
    }

    return;
}

static void strtok_test(void)
{
    char string[] = "This is a strtok test.abcde fd.aaaaaaaa";
    char *token;
    printf("string:%s\n", string);
    token = strtok(string, ".");
    if (!token) // does not have "."
        return;
    printf("token:%s\n", token);
    while ((token = strtok(NULL, ".")) != NULL) {
        printf("token:%s\n", token);
    }
    
}

// 将客户端发送过来的数据进行组装得到 pathname 和 open_flag
int cli_args(int argc, char **argv)
{
    if (argc != 3 || strcmp(argv[0], CL_OPEN) != 0) {
        strcpy(errmsg, "usage: <pathname> <oflag>\n");
        return (-1);
    }

    pathname = argv[1]; // save ptr to open
    oflag = argv[2];

    return (0);
}