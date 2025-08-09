#include "../common/apue.h"
#include "pthread.h"
#include <time.h>
#include <sys/time.h>

// 递归互斥量的使用, 使用线程实现过一段时间后调用某一个函数

static int makethread(void *(*fun)(void *), void *arg);
void *timeout_helper(void *arg);
void timeout(const struct timespec *when, void (*func)(void*), void *arg);
void retry(void *arg);

struct to_info {
    void (*to_fn)(void *);
    void *to_arg;
    struct timespec to_wait; // time to wait
};

#define SECTONSEC   1000000000 // s to ns

#if !defined(CLOCK_REALTIME) || defined(BSD)
#define clock_nanosleep(ID, FL, REQ, REM)   nanosleep((REQ), (REM))
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME  0
#define USECTONSEC      1000 // us to ns

void clock_gettime(int id, struct timespec *tsp)
{
    struct timeeval tv;

    gettimeofday(&tv, NULL);
    tsp->tv_sec = tv.tv_sec;
    tsp->tv_nsec = tv.tv_usec * USECTONSEC;
}
#endif

pthread_mutexattr_t attr;
pthread_mutex_t mutex;

int recursive_mutex_test1(void)
{
    int err, condition, arg;
    struct timespec when;


    err = pthread_mutexattr_init(&attr);
    ASSERT(err == 0);
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ASSERT(err == 0);
    err = pthread_mutex_init(&mutex, &attr);
    ASSERT(err == 0);

    // 继续执行
    condition = 1;
    pthread_mutex_lock(&mutex);
    if (condition) {
        // 计算要执行的时间
        clock_gettime(CLOCK_REALTIME, &when);
        when.tv_sec += 10;
        timeout(&when, retry, (void*)(unsigned long)arg); // create thread
    }
    pthread_mutex_unlock(&mutex);

    exit(0);
}

// thread entrance
void *timeout_helper(void *arg)
{
    struct to_info *tip;

    tip = (struct to_info *)arg;
    clock_nanosleep(CLOCK_REALTIME, 0, &tip->to_wait, NULL);
    (*tip->to_fn)(tip->to_arg);
    free(arg);

    return (0);
}

// 在该函数中调用创建线程的函数创建线程
void timeout(const struct timespec *when, void (*func)(void*), void *arg)
{
    struct timespec now;
    struct to_info *tip;
    int err;

    clock_gettime(CLOCK_REALTIME, &now);
    if ((when->tv_sec > now.tv_sec) || (when->tv_sec == now.tv_sec && when->tv_nsec > now.tv_sec)) {
        tip = (struct to_info *)malloc(sizeof(struct to_info));
        ASSERT(tip);
        tip->to_fn = func;
        tip->to_arg = arg;
        tip->to_wait.tv_sec = when->tv_sec - now.tv_sec;
        if (when->tv_nsec >= now.tv_nsec) {
            tip->to_wait.tv_nsec = when->tv_nsec - now.tv_nsec;
        } else {
            tip->to_wait.tv_sec--;
            tip->to_wait.tv_nsec = SECTONSEC - now.tv_nsec + when->tv_nsec;
        }
        // create thread
        err = makethread(timeout_helper, (void*)tip);
        if (err == 0) {
            return;
        } else {
            free(tip);
        }
    } else { // 打算执行的函数的时间在当前之间之前了, 直接执行即可
        (*func)(arg);
    }
}

void retry(void *arg)
{
    pthread_mutex_lock(&mutex);
    // 执行 retry 要执行的操作
    pthread_mutex_unlock(&mutex);
}


static int makethread(void *(*fun)(void *), void *arg)
{
    int err;
    pthread_t tid;
    pthread_attr_t attr;
    int state;

    err = pthread_attr_init(&attr);
    if (err != 0)
        return (err);
    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (err == 0)
        err = pthread_create(&tid, &attr, fun, arg);
    // err = pthread_attr_getdetachstate(&attr, &state);
    // printf("detach state:%d\n", state);

    pthread_attr_destroy(&attr);

    return (err);
}

