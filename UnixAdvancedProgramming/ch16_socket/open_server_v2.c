/**
 * @fun 服务器作为守护进程运行,服务所有客户端请求
 * 两个无关进程之间如果传递文件描述符
 * 客户端与服务端交互的协议仍然与 v1 相同
*/
#include <sys/uio.h> // struct iovec
#include <syslog.h>

#include "ch16.h"
#include  "../ch17_IPC/ch17.h"


/***************************************************************
 * Macro
 ***************************************************************/
#define NALLOC      10      // client structs to alloc/realloc for
/***************************************************************
 * Public variable
 ***************************************************************/
int debug, log_to_stderr; // nonzero if interactive(not daemon)
char errmsg[MAXLINE];
char *pathname; // of file to open for client
int oflag;
Client *client;      // ptr to malloc'ed array
int client_size;     // entries in client[] array

/**
 * @fun 为保存客户端连接的数组分配空间
 * TODO:
 * 优化:可以使用链表
*/
static void client_alloc(void)
{
    if (client == NULL) // 第一次分配内存
        client = (Client*)malloc(NALLOC * sizeof(Client));
    else // 扩容
        client = (Client*)realloc(client, (client_size = NALLOC) * sizeof(Client));
    
    if (client == NULL)
        err_sys("Can't alloc for client array");
    
    // initialize the new entries
    for (int i = client_size; i < client_size + NALLOC; i++)
        client[i].fd = -1;
    
    client_size += NALLOC;
}


/**
 * @fun 有新的客户端连接
*/
static int client_add(int fd, uid_t uid)
{
    int i;

    if (client == NULL)
        client_alloc();
    
again:
    for (int i = 0; i < client_size; ++i) {
        if (client[i].fd == -1) { // 找到了可以插入的位置, 插入 时间复杂度:O(n)
            client[i].fd = fd;
            client[i].uid = uid;
            // TODO:
            // 整个函数只有这里可以退出, 错误处理不够
            return (i); // return index in client array
        }
    }

    // client array full time to realloc for more
    client_alloc();

    goto again;
}

/**
 * @fun 删除一个客户端连接
*/
static void client_del(int fd)
{
    for (int i = 0; i < client_size; ++i) {
        if (client[i].fd == fd) {
            client[i].fd = -1;
            return;
        }
    }
    err_sys("Can't find a client fd:%d\n", fd);
}

/***************************************************************
 * Public Function
 ***************************************************************/
int start_open_server_v2(int argc, char *argv[])
{
    int c;

    log_open("open.serv", LOG_PID, LOG_USER);
    opterr = 0; // don't want getopt() writing to stderr 

    while ((c = getopt(argc, argv, "d")) != EOF) {
        switch (c) {
            case 'd': // 打开调试功能
                debug = log_to_stderr = 1;
                break;
            case '?':
                err_quit("unrecognized option -%c", optopt);
            default:
                break;
        }
    }

    if (debug == 0)
        daemonized("opend"); // 服务器进程作为守护进程运行
    loop1(); // never return
}

/**
 * @fun 使用 select 实现死讯环            
*/
void loop1(void)
{

}

/**
 * @fun 使用 poll() 实现
*/
void loop2(void)
{

}
void handle_request(char *, int, int, uid_t);
