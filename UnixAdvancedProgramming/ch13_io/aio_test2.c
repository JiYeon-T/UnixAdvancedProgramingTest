#include "../common/apue.h"
#include <aio.h>
#include <ctype.h>
#include <fcntl.h>
#include <aio.h> // -lrt
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include <sys/syscall.h>
#include <linux/aio_abi.h> // sudo apt-get install libaio-dev

/**
 * 
 * aio 现成的两个库: a.POSIX aio; 2. liaio
 * 
 * POSIX AIO 是一个用户级的实现, 它在多个线程中执行正常的阻塞 IO, 因此出现了 IO异步的感觉，主要原因是：
 * a.适用于任何文件西哦图嗯
 * b.在任何操作系统上都工作(gnu 的 libc (Glibc AIO)是可移植的)
 * c.它适用于启用了缓冲的文件(即没有开启 O_DIRECT)
 * 主要缺点是:队列深度收到选择线程数量的限制，
 * 还会影响内核和磁盘调度程序看到的 I/O
 * 
 * Glibc aio 其实是用户层使用线程模拟的异步 IO, 缺点是占用线程资源而且受可用线程的数量限制
 * Linux 2.6 版本zhhoiu有了 "libaio"
 * 完全是内核级别的异步 IO, IO 请求完全由底层自由调度(以最佳次序的磁盘调度方式)
 * 
 * 
 * libaio:
 * 缺点是:
 * 想要使用该种方式的文件必须支持以 O_DIRECT 标志为打开, 然而并不是所有的文件系统都支持
 * 如果没有使用 O_DIRECT 标志位打开文件, 它可能仍然正常工作, 
 * 但它可能不是异步的了, 而变成了阻塞
 * 安装方式:sudo apt-get install libaio-dev
 * 
 * 
 * 异步 IO 通知机制:
 * 信号处理
 * 线程回调
 * 
 */

#define BUFFER_SIZE     1024
const int AIO_LIST_SIZE = 2;

/**
 * @brief aio_read/aio_write
 * 
*/
void aio_read_test1(void)
{
    struct aiocb rb; // aio read block
    int fd, ret, counter;
    const char *in_file_path = "in.txt";

    fd = open(in_file_path, O_RDONLY);
    if (fd < 0) {
        perror(in_file_path);
    }

    bzero(&rb, sizeof(rb)); // 清空结构体

    // 填充 rb 结构体
    rb.aio_buf = malloc(BUFFER_SIZE + 1);
    rb.aio_nbytes = BUFFER_SIZE;
    rb.aio_fildes = fd;
    rb.aio_offset = 0;

    // 进行异步读操作, 立刻返回，不阻塞
    // 通过 gdb 调试发现 aio_read() 内核会创建新的线程去操作
    ret = aio_read(&rb);
    if (ret < 0) {
        perror("aio_read");
        exit(1);
    }

    // 循环等待异步读操作的结果
    counter = 0;
    while (aio_error(&rb) == EINPROGRESS) {
        printf("第%d次 loop\n", ++counter);
    }

    // aio_error != EINPROGRESS,异步读操作完成后, 获取 read 返回值
    ret = aio_return(&rb);
    printf("read ret:%d\n", ret);

    return 0;
}

/**
 * @brief 创建异步读请求
 * 
*/
static int aio_read_file(struct aiocb *pcb, int fd, int offset, int size)
{
    int ret;

    bzero(pcb, sizeof(struct aiocb));

    pcb->aio_buf = (volatile void *)malloc(size + 1); // +1  这里为什么是这个样子的???
    pcb->aio_nbytes = size;
    pcb->aio_fildes = fd;
    pcb->aio_offset = offset;

    ret = aio_read(pcb);
    if (ret < 0) {
        perror("aio read error");
        exit(1);
    }
}

void aio_suspend_test(void)
{
    struct aiocb cb1, cb2;
    int fd1, fd2, ret;
    int i = 0;
    struct aiocb* aiocb_list[AIO_LIST_SIZE];

    fd1 = open("in.txt", O_RDONLY);
    if (fd1 < 0) {
        perror("in.txt");
    }
    fd2 = open("in2.txt", O_RDONLY);
    if (fd2 < 0) {
        perror("in2.txt");
    }

    // 添加读请求
    aio_read_file(&cb1, fd1, 0, BUFFER_SIZE);
    aiocb_list[0] = &cb1;
    aio_read_file(&cb2, fd2, 0, BUFFER_SIZE);
    aiocb_list[1] = &cb2;

    // 阻塞, 知道请求完成才会执行嗯后买你的语句, 
    //这里有问题, 至少要等待所有的都异步请求都完成, 这里仅仅是测试使用
    aio_suspend((const struct aiocb *)aiocb_list, AIO_LIST_SIZE, NULL);
    printf("fd1 read:%d\n", aio_return(&cb1));
    printf("fd2 read:%d\n", aio_return(&cb2));

    close(fd1);
    close(fd2);

    return;
}

/**
 * @brief 创建异步读请求
 * 
*/
static int lio_read_file(struct aiocb *pcb, int fd, int offset, int size)
{
    int ret;

    bzero(pcb, sizeof(struct aiocb));

    pcb->aio_buf = (volatile void *)malloc(size + 1);
    pcb->aio_nbytes = size;
    pcb->aio_fildes = fd;
    pcb->aio_offset = offset;
    pcb->aio_lio_opcode = LIO_READ;
}

/**
 * @brief aio 同时为我们提供了一个可以发起多个或者多种 IO 请求的接口 lio_listio()
 *      这个函数效率很高, 因为只序oii系统调用(一次内核上下文切换)就可以完成大量的 IO 操作
 *      canshu  mode:LIO_WAIT/LIO_NOWAIT
 * 
*/
void aio_lio_listio_test(void)
{
    struct aiocb cb1, cb2;
    int fd1, fd2, ret;
    int i = 0;
    struct aiocb* aiocb_list[AIO_LIST_SIZE];

    fd1 = open("in.txt", O_RDONLY);
    if (fd1 < 0) {
        perror("in.txt");
    }
    fd2 = open("in2.txt", O_RDONLY);
    if (fd2 < 0) {
        perror("in2.txt");
    }

    // 添加读请求
    lio_read_file(&cb1, fd1, 0, BUFFER_SIZE);
    aiocb_list[0] = &cb1;
    lio_read_file(&cb2, fd2, 0, BUFFER_SIZE);
    aiocb_list[1] = &cb2;
    
    // LIO_NOWAIT:请求入队就返回
    // LIO_WAIT:所有请求结束才返回
    lio_listio(LIO_WAIT, (struct aiocb * const)aiocb_list, AIO_LIST_SIZE, NULL);
    printf("lio fd1 read:%d\n", aio_return(&cb1));
    printf("lio fd2 read:%d\n", aio_return(&cb2));

    close(fd1);
    close(fd2);

    return;
}


/**
 * @brief 信号处理函数
 *        TODO:
 *        使用有什么比较好的使用方式吗? 在里面做什么比较好
 * 
*/
static void aio_sig_handler(int signo)
{
    printf("异步操作完成, 收到通知\n");
}

/**
 * @brief 发出异步请求前就设置好请求完成后给线程发送的信号
 * 
 * @note signal() 不同平台实现有差异, 而且来信号时不能带参数, 
 *       推荐使用 sigaction(), 信号回调中可以带参数
 * 
*/
void aio_signal_test(void)
{
    struct aiocb cb;
    int fd, ret;
    int i = 0;

    fd = open("in.txt", O_RDONLY);
    if (fd < 0) {
        perror("in.txt");
    }

    bzero(&cb, sizeof(cb));
    cb.aio_buf = (volatile void *)malloc(BUFFER_SIZE + 1);
    cb.aio_nbytes = BUFFER_SIZE;
    cb.aio_fildes = fd;
    cb.aio_offset = 0;
    cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL; // 发送信号方式通知
    cb.aio_sigevent.sigev_signo = SIGINT; // 要发送的信号, 也可以选择其他信号
    cb.aio_sigevent.sigev_value.sival_ptr = &cb;

    // 安装信号
    signal(SIGIO, aio_sig_handler);
    // signal(SIGINT, aio_sig_handler);
    // signal(SIGTERM, aio_sig_handler);
    // sigaction();

    ret = aio_read(&cb);
    if (ret < 0) {
        perror("aio_read error\n");
        exit(1);
    }

    // 挂起线程, 等待异步 IO 完成信号
    printf("sleep\n");
    sleep(5);
    printf("wake up\n");
    close(fd);

    return;
}


// typedef struct sigevent {
// 	sigval_t sigev_value;
// 	int sigev_signo;
// 	int sigev_notify;
//     // union 这种写法要好好学习
// 	union {
// 		int _pad[SIGEV_PAD_SIZE];
// 		int _tid;

// 		struct {
// 			void (*_function)(sigval_t);
// 			void *_attribute;	/* really pthread_attr_t */
// 		} _sigev_thread;
// 	} _sigev_un;
// } sigevent_t;

/**
 * @brief 异步操作完成后的线程入口函数
 * 
*/
static void aio_handler(sigval_t sigval)
{
    struct aiocb *pcb;
    int ret;

    printf("异步操作完成, 收到通知\n");
    pcb = (struct aiocb *)sigval.sival_ptr; // 获取 aiocb 结构体

    if (aio_error(pcb) == 0) { // 异步操作成功
        ret = aio_return(pcb);
        printf("异步操作返回值:%d\n", ret);
    }

    // 线程
    while (1) {
        printf("正在执行回调函数\n");
        sleep(1);
    }
}

/**
 * @brief 线程回调:即当进程是哦读奥异步操作完成的通知时, 创建新的线程来执行之前设置的回调函数
 *        无需安装信号
 * 
 * @note 在 Glibc AIO 的实现中, 用多线程同步来模拟异步 IO, 该代码涉及了 3 个线程
 *       a. 主线程, 调用 aio_read() 发出异步读请求
 *       b. aio_read() 会分配一个子线程去阻塞读该 IO, 直到结束发通知
 *       c. 在通知线程中执行之前设置的线程回调函数
 * 
 *       实际上, 为了避免线程的频繁创建,销毁, 当有多个异步操作请求时, Glibc 会使用线程池, 单原理上不会变
 *       之前设置的"线程回调函数"是在单独的线程中运行的
 *       Glibc aio 广受非议, 存在一些难以忍受的缺陷和 bug，极不推荐使用
 *  
 **/
void aio_signal_thread_test(void)
{
    struct aiocb cb;
    int fd, ret;
    int i = 0;

    fd = open("in.txt", O_RDONLY);
    if (fd < 0) {
        perror("in.txt");
    }

    bzero(&cb, sizeof(cb));
    cb.aio_buf = (volatile void *)malloc(BUFFER_SIZE);
    cb.aio_nbytes = BUFFER_SIZE;
    cb.aio_fildes = fd;
    cb.aio_offset = 0;
    cb.aio_sigevent.sigev_notify = SIGEV_THREAD; // 设置异步操作完成后, 线程通知方式
    cb.aio_sigevent.sigev_notify_function = aio_handler; // 线程入口函数
    cb.aio_sigevent.sigev_value.sival_ptr = &cb; // 传入 cb 结构体
    cb.aio_sigevent.sigev_notify_attributes = NULL; // 线程 attribute 信息

    //异步读请求
    ret = aio_read(&cb);
    if (ret < 0) {
        perror("aio_read error");
        exit(1);
    }

    // 调用线程继续执行
    while (1) {
        printf("主线程继续运行\n");
        sleep(1);
    }

    close(fd);

    return;
}


/**
 * @brief 使用 libaio 而不是 Glibc AIO
 *
 * 
*/
void libaio_test1(void)
{
    //TODO:
    //该知识点需要配合 syscall() 学习:
    //都与 Linux 系统调用有关, 需要学习
    //https://blog.csdn.net/m0_51551385/article/details/125116618
    //https://blog.csdn.net/notbaron/article/details/79499074?utm_medium=distribute.pc_relevant.none-task-blog-2~default~baidujs_baidulandingword~default-0-79499074-blog-49803855.235^v43^pc_blog_bottom_relevance_base6&spm=1001.2101.3001.4242.1&utm_relevant_index=3
    // SYS_io_destroy
    // __NR_fchmod
}

