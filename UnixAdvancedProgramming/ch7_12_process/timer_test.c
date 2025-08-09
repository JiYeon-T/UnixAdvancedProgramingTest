/**
 * @file:
 * @fun: unix/posix 下的各种软件定时器的使用
*/
#include "../common/apue.h"

// 挂起调用进程直到超时或者被信号唤起该进程
unsigned int sleep(unsigned int seconds);
int nanosleep(const struct timespec *reqtp, struct timespec *remtp); // 内部实现与信号无关,可以放心使用信号
// int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *reqtp,
//                     const struct timespec *remtp);


// 自己使用 alarm() 实现 sleep()
unsigned int sleep_m1(unsigned int seconds)
{

}

void sleep_test1(void)
{
    int ret;
    struct timespec delay_time;

    printf("sleep 1s\n");
    delay_time.tv_sec = 1;
    delay_time.tv_nsec = 1;
    nanosleep(&delay_time, NULL);

    printf("sleep 1s\n");
    ret = clock_nanosleep(CLOCK_REALTIME, 0, &delay_time, NULL);
    if (ret != 0) {
        printf("clock_nanosleep failed, ret:%d errno:%d\n", ret, errno);
    }

    printf("end\n");
}