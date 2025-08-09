#include "../common/apue.h"
#include <aio.h>
#include <ctype.h>
#include <fcntl.h>
#include <aio.h> // -lrt 必须使用该选项对 rt 进行链接
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>


#define AIO_BUFF_SIZE       256 //4096
#define AIO_BUFF_NUM        8 // number that can accept aio operation request

static unsigned char translate(unsigned char c);
static const char *_get_op_str(int op);
static void aio_sm_transfer_to(int worker_index, int op);


// 这里类似于一个状态机?
enum rwop {
    UNUSED = 0,
    READ_PENDING = 1,
    WRITE_PENDING = 2
};



typedef struct _aio_buf {
    enum rwop op; // 状态机的当前状态
    int last; // 当前是否是最后一包数据
    struct aiocb aiocb; // aio control block
    unsigned char data[AIO_BUFF_SIZE];
} aio_buf_t;

unsigned char buf[AIO_BUFF_SIZE] = {0}; // translate_buffer
aio_buf_t aio_buf[AIO_BUFF_NUM] = {0}; // 异步 IO 使用的 buffer, 相当于是个异步 IO 队列

/**
 * @fun aio test1:transfer file character, 模糊化处理
 * 
 * @param[in] argc[1] input file name
 * @param[in] argc[2] output file name
 * @ret
*/
void aio_test1(int argc, char *argv[])
{
    int ifd, ofd, i, j, n, err;
    int aio_op_num; //当前正在进行的 aio request 的个数
    struct stat sbuf;
    const struct aiocb* aiolist[AIO_BUFF_NUM];
    off_t off = 0;
    bzero(aio_buf, sizeof(aio_buf));
    bzero(buf, sizeof(buf));


    if (argc != 3)
        err_ret("usage:test infileName outFileName");
    if ((ifd = open(argv[1], O_RDONLY)) < 0)
        err_ret("open file:%s failed", argv[1]);
    if ((ofd = open(argv[2], O_CREAT | O_RDWR | O_TRUNC, FILE_MODE)) < 0)
        err_ret("open file:%s failed", argv[2]);
    if (fstat(ifd, &sbuf) < 0)
        err_sys("fstat failed");
    
    // intialize the aio operation buffers
    for (i = 0; i < AIO_BUFF_NUM; ++i) {
        // aio_buf[i].op = UNUSED;
        aio_sm_transfer_to(i, UNUSED);
        aio_buf[i].aiocb.aio_buf = aio_buf[i].data;
        aio_buf[i].aiocb.aio_sigevent.sigev_notify = SIGEV_NONE;
        aiolist[i] = NULL;
    }

    aio_op_num = 0;
    for (;;) {
        for (i = 0; i < AIO_BUFF_NUM; ++i) { // 遍历异步 IO 队列
            printf("i:%d  op:%s\n", i, _get_op_str(aio_buf[i].op));
            switch(aio_buf[i].op) {
                case UNUSED: {
                    // Read from the input file if more data remains unread
                    // 如果还有文件可以读就进入 read_pending 状态
                    if (off < sbuf.st_size) {
                        // aio_buf[i].op = READ_PENDING;
                        aio_sm_transfer_to(i, READ_PENDING);
                        aio_buf[i].aiocb.aio_fildes = ifd;
                        aio_buf[i].aiocb.aio_offset = off;
                        off += AIO_BUFF_SIZE;
                        if (off >= sbuf.st_size)
                            aio_buf[i].last = 1;
                        aio_buf[i].aiocb.aio_nbytes = AIO_BUFF_SIZE;
                        // aiocb 的其他成员已经在上面初始化
                        // 用户发出 aio read request
                        if (aio_read(&aio_buf[i].aiocb) < 0)
                            err_sys("aio read failed");
                        aiolist[i] = &aio_buf[i].aiocb;
                        ++aio_op_num;
                    }
                    break;
                }
                case READ_PENDING: {
                    // 获取异步操作的结果
                    if ((err = aio_error(&aio_buf[i].aiocb)) == EINPROGRESS) // aio_error() 获取异步操作的完成状态
                        continue;
                    if (err != 0) {
                        if (err == -1)
                            err_sys("aio_error failed");
                        else
                            err_exit("reaad failed");
                    }

                    // a read is complete, translete the buffer and write it to file
                    // 只有异步操作成功才可以调用 aio_return() 来获取异步操作的返回值
                    if ((n = aio_return(&aio_buf[i].aiocb)) < 0)
                        err_sys("aio return failed");
                    if (n != AIO_BUFF_SIZE && !aio_buf[i].last) // 读失败
                        err_quit("short read (%d/%d)", n, AIO_BUFF_SIZE);

                    // 读成功, translate
                    for (j = 0; j < n; ++j)
                        aio_buf[i].data[j] = translate(aio_buf[i].data[j]);
                    /**模拟应用程序进行数据处理的耗时，实际中数据处理耗时较少，主要是 IO 耗时很多, 怎么模拟 io 耗时**/
                    usleep(1000 * 1000);
                    printf("aio worker[%d] trans:%d\n", i, n);

                    // 用户发出 aio write request
                    // 进入 write_pending 状态
                    // aio_buf[i].op = WRITE_PENDING;
                    aio_sm_transfer_to(i, WRITE_PENDING);
                    aio_buf[i].aiocb.aio_fildes = ofd;
                    aio_buf[i].aiocb.aio_nbytes = n;
                    if (aio_write(&aio_buf[i].aiocb) < 0)
                        err_sys("aio_write failed");
                    break;
                }
                case WRITE_PENDING: {
                    // 获取异步操作结果
                    if ((err = aio_error(&aio_buf[i].aiocb)) == EINPROGRESS)
                        continue;
                    if (err != 0) {
                        if (err == -1)
                            err_sys("aio_erro failed");
                        else
                            err_exit("write failed");
                    }
                    // write is complete
                    if ((n = aio_return(&aio_buf[i].aiocb)) < 0)
                        err_sys("aio_return failed");
                    if (n != aio_buf[i].aiocb.aio_nbytes)
                        err_quit("short write (%d/%ld)", n, aio_buf[i].aiocb.aio_nbytes);
                    aiolist[i] = NULL;
                    // aio_buf[i].op = UNUSED;
                    aio_sm_transfer_to(i, UNUSED);
                    --aio_op_num;
                    break;
                }
            }
        }

        if (aio_op_num == 0) {
            // 文件转换结束
            if (off >= sbuf.st_size) {
                printf("file transfer finished\n");
                break; // 这里 break for 循环
            }
        } 
        else {
            // suspend 阻塞进程
            if (aio_suspend(aiolist, AIO_BUFF_NUM, NULL) < 0)
                err_sys("aio_suspend failed");
            // 这里可以选择不阻塞去做其他事情吗???
            // 应该是可以的, 异步操作完成后, 通过信号的方式通知该进程 aio_test5
        }
    }

    // 不等待, 强制写入存储
    aio_buf[0].aiocb.aio_fildes = ofd;
    if (aio_fsync(O_SYNC, &aio_buf[0].aiocb) < 0)
        err_sys("aio_fsync failed");

    return;
}

// 模糊化处理字符
static unsigned char translate(unsigned char c)
{
    if (isalpha(c)) {
        if (c >= 'n')
            c -= 13;
        else if (c >= 'a')
            c += 13;
        else if (c >= 'N')
            c -= 13;
        else
            c += 13;
    }

    return (c);
}

static const char *_get_op_str(int op)
{
    switch (op) {
        CASE_RET_STR(UNUSED, "UNUSED");
        CASE_RET_STR(READ_PENDING, "READ_PENDING");
        CASE_RET_STR(WRITE_PENDING, "WRITE_PENDING");
    }
}

/**
 * @fun 切换 aio 每一个 worker 的状态
 * 
 * @param[in] worker_index 数组 aio_buf 索引
 * @param[in] op rwop 中的元素
 * 
*/
static void aio_sm_transfer_to(int worker_index, int op)
{
    printf("aio worker[%d] transfer from [%s] to -> [%s]\n", worker_index, 
        _get_op_str(aio_buf[worker_index].op), _get_op_str(op));
    aio_buf[worker_index].op = op;
}

void char_process_test(int argc, char *argv[])
{
    int ifd, ofd, i, n, nw;

    if (argc != 3)
        err_ret("usage:test infileName outFileName");
    if ((ifd = open(argv[1], O_RDONLY)) < 0)
        err_ret("open file:%s failed", argv[1]);
    if ((ofd = open(argv[2], O_CREAT | O_RDWR | O_TRUNC, FILE_MODE)) < 0)
        err_ret("open file:%s failed", argv[2]);

    while ((n = read(ifd, buf, AIO_BUFF_SIZE)) > 0) {
        for (i = 0; i < n; ++i)
            buf[i] = translate(buf[i]);
        if ((nw = write(ofd, buf, n)) != n) {
            if (nw < 0)
                err_sys("write failed");
            else
                err_quit("short write (%d/%d)", nw, n)
        }
        printf("trans:%d\n", n);
    }

    fsync(ofd);
    return;
}


