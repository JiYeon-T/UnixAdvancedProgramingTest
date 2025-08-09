#include "../common/apue.h"
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <poll.h>
#include <sys/select.h>


//有名管道
#define FIFO_NAME       "/tmp/fiofo_test"
#define FIFO_NAME2       "/tmp/fiofo_test2"


/**
 * @brief 发送线程
 * 
*/
void *sender_thread(void *args)
{
    int ret;
    int fd;
    int send_len;
    // ret = mkfifo(FIFO_NAME, 0777);
    // ASSERT(ret >= 0);
    
    // 只写/只读方式打开 + O_NOBLOCK(默认不设置, 即阻塞) 会阻塞
    fd = open(FIFO_NAME, O_RDWR);
    // ASSERT(ret >= 0);
    printf("fd:%d\n", fd);
    while (1) {
        char buf[] = "123456";
        send_len = write(fd, buf, sizeof(buf));
        printf("send:%s len:%d\n", buf, send_len);
        sleep(1);
    }

    close(fd);

    return NULL;
}

// 发送线程
void *sender_thread2(const char *fifo_name)
{
    int ret;
    int fd;
    int send_len;
    // ret = mkfifo(FIFO_NAME, 0777);
    // ASSERT(ret >= 0);
    
    // 只写/只读方式打开 + O_NOBLOCK(默认不设置, 即阻塞) 会阻塞
    fd = open(fifo_name, O_RDWR);
    // ASSERT(ret >= 0);
    printf("fd:%d\n", fd);
    while (1) {
        char buf[] = "123456";
        send_len = write(fd, buf, sizeof(buf));
        printf("send:%s len:%d\n", buf, send_len);
        sleep(1);
    }

    close(fd);

    return NULL;
}

/**
 * @brief 接收线程
 * 
*/
void *receiver_thread(void *args)
{
    int ret;
    int fd;
    int recv_len;
    int recv_item_idx = 0;

    ret = mkfifo(FIFO_NAME, 0777);
    // ASSERT(ret >= 0);

    fd = open(FIFO_NAME, O_RDWR);
    // ASSERT(ret >= 0);
    printf("fd:%d\n", fd);
    while (1) {
        char buf[256];
        recv_len = read(fd, buf, sizeof(buf));
        printf("recv[%d]:%s len:%d\n", ++recv_item_idx, buf, recv_len);
    }

    close(fd);

    return NULL;
}


/**
 * @brief IO 多路复用
 * 
*/
void *receiver_thread2(void *args)
{
    int ret;
    int fd1, fd2;
    int recv_len;
    int recv_item_idx = 0;
    struct pollfd pfd[2];

    ret = mkfifo(FIFO_NAME, 0777);
    // ASSERT(ret >= 0);
    ret = mkfifo(FIFO_NAME2, 0777);
    // ASSERT(ret >= 0);

    fd1 = open(FIFO_NAME, O_RDWR);
    // ASSERT(ret >= 0);
    printf("fd1:%d\n", fd1);

    fd2 = open(FIFO_NAME, O_RDWR);
    // ASSERT(ret >= 0);
    printf("fd2:%d\n", fd2);

    pfd[0].fd = fd1;
    pfd[0].events = POLLIN;
    
    pfd[1].fd = fd2;
    pfd[1].events = POLLIN;

    while (1) {
        if (poll(pfd, 2, -1) < 0) // block
            exit(-1);
        char buf[256];
        for (int i = 0; i < 2; ++i) {
            if (pfd[i].revents & POLLIN) {
                recv_len = read(pfd[i].fd, buf, sizeof(buf));
                printf("recv[%d]:%s len:%d\n", ++recv_item_idx, buf, recv_len);
            }
        }
    }

    close(fd1);
    close(fd2);

    return NULL;
}



int main(int argc, char *argv[])
{
    // sender_thread(NULL);
    sender_thread2(FIFO_NAME2);
    // receiver_thread(NULL);
    // receiver_thread2(NULL);
    return 0;
}