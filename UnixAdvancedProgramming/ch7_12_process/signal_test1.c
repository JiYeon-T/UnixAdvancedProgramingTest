#include "../common/apue.h"
#include "pthread.h"
#include <time.h>
#include <sys/time.h>
#include <signal.h>

// 1.每个线程都有自己的屏蔽字，但是信号处理程序 signal_handler() 却是进程中的所有线程共享的
// 意味着单个线程可以组织某些信号,意味着单个线程可以阻止某些信号,进程中的其他线程是共享这个行为的, 其他线程也收不到
// 2. 进程中的信号是递送到单个线程的,如果一个信号与硬件故障有关,那么它会被递送到引起该事件的线程去
// 而其他信号则被发送到任意一个线程
// 3. 进程使用 sigprocmask 函数阻止信号发送, 然而, sigprocmask 的行为在多线程的进程中没有定义, 
// 线程必须使用 pthread_sigmask()
static void *thread_fun(void *arg);

int quitflag;
sigset_t mask;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitloc = PTHREAD_COND_INITIALIZER;


void thread_signal_test1(void)
{
    int err;
    sigset_t oldmask;
    pthread_t tid;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    err = pthread_sigmask(SIG_BLOCK, &mask, &oldmask); // 主线程屏蔽这些信号
    ASSERT(err == 0);

    err = pthread_create(&tid, NULL, thread_fun, NULL);
    ASSERT(err == 0);

    pthread_mutex_lock(&lock);
    while (quitflag == 0)
        pthread_cond_wait(&waitloc, &lock);
    pthread_mutex_unlock(&lock);

    // SIGQUIT has been caught and now blocked
    quitflag = 0;

    // 恢复信号
    if ((err = sigprocmask(SIG_SETMASK, &oldmask, NULL)) < 0) {
        err_ret("reset signalset failed");
    }
    exit(0);
}

// 用于信号处理的线程, 而不是信号处理程序
static void *thread_fun(void *arg)
{
    int err, signo;

    for (;;) {
        err = sigwait(&mask, &signo);
        if (err != 0)
            err_quit("sig wait failed:%d\n", err);
        switch (signo) {
            case SIGINT:
                printf("interrupt\n");
                break;
            case SIGQUIT:
                err = pthread_mutex_lock(&lock);
                ASSERT(err == 0);
                quitflag = 1;
                printf("quit flag:%d\n", quitflag);
                err = pthread_mutex_unlock(&lock);
                ASSERT(err == 0);
                err = pthread_cond_signal(&waitloc);
                ASSERT(err == 0);
                return (0);
            default:
                printf("unknown signal:%d\n", signo);
        }
    }
}