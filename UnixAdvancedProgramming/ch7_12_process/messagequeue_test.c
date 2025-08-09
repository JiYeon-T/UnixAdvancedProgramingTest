#include "../common/apue.h"
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <pthread.h>
#include <errno.h>


/********************** Linux MQ Interface START *************************/
// mq_close()
// mq_getattr()
// mq_notify()
// mq_open()
// mq_receive()
// mq_send()
// mq_setattr()
// mq_timedreceive()
// mq_timedsend()
// mq_unlink()
/********************** Linux MQ Interface END *************************/

/********************** System V MQ Interface START *************************/
/**
*@brief创建消息队列实例
*
*Detailedfunctiondescription
*
*@param[in]name:消息队列名称
*@param[in] oflag：根据传入标识来创建或者打开一个已创建的消息队列
-O_CREAT:创建一个消息队列
-O_EXCL:检查消息队列是否存在，一般与O_CREAT一起使用
-O_CREAT|O_EXCL:消息队列不存在则创建，已存在返回NULL
-O_NONBLOCK:非阻塞模式打开，消息队列不存在返回NULL
-O_RDONLY:只读模式打开
-O_WRONLY:只写模式打开
-O_RDWR:读写模式打开
*@param[in] mode：访问权限
*@param[in] attr：消息队列属性地址
*
*@return成功返回消息队列描述符，失败返回-1，错误码存于error中
*/
// mqd_tmq_open(constchar*name,intoflag,mode_tmode,structmq_attr*attr);

/**
*@brief无限阻塞方式接收消息
*
*Detailedfunctiondescription
*
*@param[in]mqdes:消息队列描述符
*@param[in] msg_ptr：消息体缓冲区地址
*@param[in] msg_len：消息体长度，长度必须大于等于消息属性设定的最大值
*@param[in] msg_prio：消息优先级
*
*@return成功返回消息长度，失败返回-1，错误码存于error中
*/
// mqd_tmq_receive(mqd_tmqdes,char*msg_ptr,size_tmsg_len,unsigned*msg_prio);

/**
*@brief指定超时时间阻塞方式接收消息
*
*Detailedfunctiondescription
*
*@param[in]mqdes:消息队列描述符
*@param[in] msg_ptr：消息体缓冲区地址
*@param[in] msg_len：消息体长度，长度必须大于等于消息属性设定的最大值
*@param[in] msg_prio：消息优先级
*@param[in] abs_timeout：超时时间
*
*@return成功返回消息长度，失败返回-1，错误码存于error中
*/
// mqd_tmq_timedreceive(mqd_tmqdes,char*msg_ptr,size_tmsg_len,unsigned*msg_prio,conststructtimespec*abs_timeout);

/**
*@brief无限阻塞方式发送消息
*
*Detailedfunctiondescription
*
*@param[in]mqdes:消息队列描述符
*@param[in] msg_ptr：待发送消息体缓冲区地址
*@param[in] msg_len：消息体长度
*@param[in] msg_prio：消息优先级
*
*@return成功返回0，失败返回-1
*/
// mqd_tmq_send(mqd_tmqdes,constchar*msg_ptr,size_tmsg_len,unsignedmsg_prio);

/**
*@brief指定超时时间阻塞方式发送消息
*
*Detailedfunctiondescription
*
*@param[in]mqdes:消息队列描述符
*@param[in] msg_ptr：待发送消息体缓冲区地址
*@param[in] msg_len：消息体长度
*@param[in] msg_prio：消息优先级
*@param[in] abs_timeout：超时时间
*
*@return成功返回0，失败返回-1
*/
// mqd_tmq_timedsend(mqd_tmqdes,constchar*msg_ptr,size_tmsg_len,unsignedmsg_prio,conststructtimespec*abs_timeout);

/**
*@brief关闭消息队列
*
*Detailedfunctiondescription
*
*@param[in]mqdes:消息队列描述符
*
*@return成功返回0，失败返回-1
*/
// mqd_tmq_close(mqd_tmqdes);

/**
*@brief分离消息队列
*
*Detailedfunctiondescription
*
*@param[in]name:消息队列名称
*
*@return成功返回0，失败返回-1
*/
// mqd_tmq_unlink(constchar*name);
/********************** System V MQ Interface END *************************/


/** POSIX 消息队列使用 **/
// 消息队列可以用于进程间通信,也可以用于线程间通信;
// 例:线程间通信
#define TEST_QUQUE_NAME         "/test_mq" // 修改不同名字会提示 open queue failed, 加上超级用户权限 sudo ./test 就没问题
#define TEST_QUEUE_MAX_SIZE     512
#define TEST_QUEUE_MAX_ITEM_NUM 10

static pthread_t s_thread1_id;
static pthread_t s_thread2_id;
static uint8_t s_thread1_running = 0;
static uint8_t s_thread2_running = 0;
static mqd_t s_mq;
static char send_msg[10] = "hello";

void *thread1_fun(void *arg)
{
    int ret = 0;
    s_thread1_running = 1;

    while (s_thread1_running) {
        ret = mq_send(s_mq, send_msg, sizeof(send_msg), 0);
        if (ret < 0) {
            perror("mq_send error");
        }
        printf("send msg:%s\n", send_msg);
        usleep(100 * 1000);
    }
    // pthread_exit(NULL);
    printf("thread1 exit\n");
}

void *thread2_fun(void *arg)
{
    uint8_t buf[TEST_QUEUE_MAX_SIZE];
    int recv_size = 0;
    s_thread2_running = 1;
    int ret;
    struct mq_attr attr;


    while (s_thread2_running) {
        memset(&attr, 0, sizeof(struct mq_attr));
        ret = mq_getattr(s_mq, &attr);
        if (ret != -1) {
            printf("inner member num:%ld %ld %ld %ld\n", attr.mq_flags, attr.mq_maxmsg,
                    attr.mq_msgsize, attr.mq_curmsgs);
        } else {
            perror("get attr failed");
        }
        recv_size = mq_receive(s_mq, buf, sizeof(buf), NULL);
        if (recv_size < 0) {
            perror("mq_receive failed\n");
        }
        printf("recv msg:%s\n", buf);
        usleep(10000 * 1000);
    }
    printf("thread2 exit\n");
}

// gcc main.c send_queue_test.c ../common/apue.c -o test -lpthread -lrt
int mq_test1(void)
{
    int ret;
    struct mq_attr attr;

    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = TEST_QUEUE_MAX_ITEM_NUM; //????????????????????? 为什么一直是 5
    attr.mq_msgsize = TEST_QUEUE_MAX_SIZE;
    attr.mq_flags = 0;
    s_mq = mq_open(TEST_QUQUE_NAME, O_CREAT | O_RDWR, 0777, &attr);
    if (s_mq == -1) {
        perror("create mq failed\n");
        return -1;
    }

    ret = pthread_create(&s_thread1_id, NULL, thread1_fun, NULL);
    if (ret != 0) {
        perror("create thread1 failed\n");
        return -2;
    }

    ret = pthread_create(&s_thread2_id, NULL, thread2_fun, NULL);
    if (ret != 0) {
        perror("create thread2 failed\n");
        return -3;
    }

    while (1) {
        pause();
    }

    return 0;
}


// 方案1：入队, 在发送回调中释放内存并且判断队列是否有空间, 没有空间就阻塞, 有空间就入队并出队一个元素

// 发送回调函数
typedef void (*send_cb_t)(uint8_t send_result);


void send_data(uint8_t *p_data, uint16_t len, send_cb_t cb)
{

}


