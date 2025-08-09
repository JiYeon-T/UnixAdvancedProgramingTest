#include "../common/apue.h"

#include <pthread.h>
#include <time.h>
#include <limits.h>


extern int pthread_equal(pthread_t tid1, pthread_t tid2);
extern pthread_t pthread_self(void);
extern int pthread_create(pthread_t *restrict tidp, const pthread_attr_t *restrict attr,
                    void *(*start_rtn)(void*), void * restrict arg);
//2.自己调用 pthread_exit();3.同一进程中的其他线程调用 pthread_cancel()
extern void pthread_exit(void *rval_ptr); // 线程退出的三种方式:1.函数运行结束退出; 
extern int pthread_join(pthread_t thread, void **rval_ptr); // 阻塞等待线程结束
extern int pthread_cancel(pthread_t tid);
// extern void pthread_cleanup_push(void (*rtn)(void*), void *arg); // 线程清理处理程序
// extern void pthread_cleanup_pop(int execute);
int pthread_detach(pthread_t tid); // 分离一个线程,作用:线程结束后内存资源立马被系统收回
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t * restrict attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t * restrict mutex, const struct timespec *restrict tsptr);

int pthread_rwlock_init(pthread_rwlock_t * restrict rwlock, const pthread_rwlockattr_t * restrict attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t * restrict rwlock, const struct timespec * restrict tsptr);
int pthread_rwlock_timedwrlock(pthread_rwlock_t * restrict rwlock, const struct timespec * restrict tsptr);

////因为POSIX标准只规定了接口长什么样子，没规定怎么实现，所以pthread_cond_t这个数据类型可能被实现为结构体，为了最大化可移植性，就搞了个init函数来动态初始化
int pthread_cond_init(pthread_cond_t * restrict cond, const pthread_condattr_t * restrict attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t  * restrict cond, pthread_mutex_t * restrict mutex); //pthread_cond_wait()函数可以被pthread_cond_signal()或者是pthread_cond_broadcast()函数唤醒
int pthread_cond_timedwait(pthread_cond_t * restrict cond, pthread_mutex_t * mutex, const struct timespec * restrict tsptr);
int pthread_cond_signal(pthread_cond_t *cond); //pthread_cond_signal()可以唤醒至少一个线程
int pthread_cond_breadcast(pthread_cond_t *cond); //而pthread_cond_broadcast()则是唤醒等待该条件满足的所有线程

int pthread_spin_init(pthread_spinlock_t *lock, int shared); // 不阻塞(阻塞会导致线程休眠,调度器重新调度),忙等
int pthread_spin_destroy(pthread_spinlock_t *lock);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);

//主线程子线程同时阻塞等待
int pthread_barrier_init(pthread_barrier_t * restrict barrier, const pthread_barrierattr_t * restrict attr, unsigned int count);
int pthread_barrier_destroy(pthread_barrier_t * restrict barrier);
int pthread_barrier_wait(pthread_barrier_t *barrier);

// 线程/互斥量/条件/屏障等属性设置
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_getdetachstate(const pthread_attr_t * restrict attr, int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t * restrict attr, int detachstate);
int pthread_attr_getstack(const pthread_attr_t * restrict attr, void ** restrict stackaddr, size_t * restrict stacksize);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t * restrict attr, size_t * restrict stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getguardsize(const pthread_attr_t * restrict attr, size_t * restrict size);
int pthread_attr_setguardsize(pthread_attr_t * attr, size_t guardsize);

// 设置锁的进程共享属性
int pthread_mutexattr_getshared(const pthread_mutexattr_t * restrict attr, int * restrict pshared);
int pthread_mutexattr_setshared(const pthread_mutexattr_t * attr, int shared);
// 处理多个进程间共享锁时, 持锁进程退出时没有释放锁的情况
int pthread_mutexattr_getrobust(const pthread_mutexattr_t * restrict attr, int * restrict robust);
// int pthread_mutexattr_setrobust(const pthread_mutexattr_t * restrict attr, int robust);
int pthread_mutex_consistent(pthread_mutex_t *mutex);
int pthread_mutex_gettype(const pthread_mutexattr_t *restrict attr, int * restrict type);
int pthread_mutex_settype(pthread_mutexattr_t *attr, int type);
int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t * restrict attr, int * restrict pshared);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);

int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_getpshared(const pthread_condattr_t * restrict attr, int * restrict pshared);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
int pthread_condattr_getclock(const pthread_condattr_t * restrict attr, clockid_t * restrict clock_id);
int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id);

int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
int pthread_barrierattr_getpshared(const pthread_barrierattr_t * restrict attr, int * restrict pshared);
int pthread_barrierattr_setpshared(pthread_barrierattr_t * restrict attr, int pshared);

// 虽然标准 IO 例程可能从他们各自的内部数据结构的角度出发是线程安全的
// 但有时把锁开放给应用程序是十分有必要的,折允许应用程序将多个 IO 操作组合成一个原子操作
int ftrylockfile(FILE *fp); // std 标准库的接口
extern void flockfile(FILE *fp);
extern void funlockfile(FILE *fp);
int getchar_unlocked(void);
int getc_unlocked(FILE *fp);
int putchar_unlocked(int c);
int putc_unlocked(int c, FILE *fp);

// 线程特定数据/线程私有数据
int pthread_key_create(pthread_key_t *keyp, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
int pthread_once(pthread_once_t *initflag, void (*initfn)(void));
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

// 线程取消, pthread_cancel() 取消线程后, 线程何时真正结束
int pthread_setcancelstate(int state, int *oldstate); // 默认是推迟取消,到达指定取消点后线程才会取消
void pthread_testcancel(void); // 手动添加取消点
int pthread_setcanceltype(int type, int *oldvalue); // 设置推迟取消/异步取消,

// 线程和信号
int pthread_sigmask(int how, const sigset_t * restrict set, sigset_t * restrict oset);
int sigwait(const sigset_t * restrict set, int * restrict signop);
int pthread_kill(pthread_t thread, int signo);

// fork 处理程序
int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

// 线程与 IO
// ssize_t pread(int fd, void *buf, size_t count, off_t offset);
// ssize_t pwrite(int fd, void *buf, size_t count, off_t offset);










static void printids(const char *s)
{ 
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    printf("%s pid:%lu tid:%lu 0x%lx\n", s, (unsigned long)pid, 
            (unsigned long)tid, (unsigned long)tid);
}

static pthread_t ntid1;
void *thread_fun1(void *arg);

void thread_test1(void)
{
    // printf("thread id:%ld\n", pthread_self());
    int err;

    err = pthread_create(&ntid1, NULL, thread_fun1, NULL);
    if (err != 0) {
        err_ret("Can't create new thread");
    }
    printids("main thread");
    sleep(1);
    exit(0);
}

void *thread_fun1(void *arg)
{
    printids("child thread");
    return NULL;
}

void *thread_fun2(void *arg);
void *thread_fun3(void *arg);

void thread_exit_test2(void)
{
    int err;
    pthread_t tid1, tid2;
    void *tret;

    err = pthread_create(&tid1, NULL, thread_fun2, NULL);
    if (err != 0)
        err_ret("create thread1 failed");
    err = pthread_create(&tid2, NULL, thread_fun3, NULL);
    if (err != 0)
        err_ret("create thread1 failed");

    err = pthread_join(tid1, &tret); // 阻塞等待线程结束
    if (err != 0)
        err_ret("can't join with thread1")
    printf("thread1 exit, code:%ld\n", (long)tret);
    err = pthread_join(tid2, &tret);
    if (err != 0)
        err_ret("can't join with thread1")
    printf("thread2 exit, code:%ld\n", (long)tret);
    exit(0); // 主线程退出, 进程退出
}
// 线程正常退出
void *thread_fun2(void *arg)
{
    printf("thread2 running..\n");
    return ((void*)1);
}

// 线程自身调用 pthread_exit() 退出
void *thread_fun3(void *arg)
{
    int ix = 0;
    printf("thread3 running ix addr:%p\n", &ix);
    while (1) {
        if (ix == 5) {
            printf("thread2 exit");
            pthread_exit((void*)2);
        }
        sleep(1);
        printf("ix:%d\n", ++ix);
    }
}

// pthread_exit() 返回的参数需要是堆内存/全局变量,不可以是栈内存
// 错误示范:
struct foo {
    int a, b, c, d;
};
static void printfoo(const char *s, const struct foo *fp);
void *thread_fun4(void *arg);

void thread_exit_test3(void)
{
    pthread_t tid;
    int ret;
    struct foo *fp;

    ret = pthread_create(&tid, NULL, thread_fun4, NULL);
    if (ret != 0)
        err_ret("create thread failed");

    ret = pthread_join(tid, (void*)&fp);
    if (ret != 0)
        err_ret("join thread failed");
    
    sleep(1);
    printfoo("main thread", fp);
    exit(0);
}

static void printfoo(const char *s, const struct foo *fp)
{
    printf("%s: fp:0x%lx\n", s, (unsigned long)fp); // 打印指针的值
    printf("a:%d b:%d c:%d d:%d\n\n", fp->a, fp->b, fp->c, fp->d);
}

void *thread_fun4(void *arg)
{
    struct foo f;
    f.a = 1;
    f.b = 2;
    f.c = 3;
    f.d = 4;
    printf("thread fun4 running\n");
    printfoo("child thread", &f);
    // 可以改为堆内存/全局变量
    pthread_exit((void*)&f); // pthread_exit() 线程退出传栈内存
}


void cleanup(void *arg);

void *thread_fun5(void *arg)
{
    printf("thread5 start\n");
    pthread_cleanup_push(cleanup, "thread5 first handler");
    pthread_cleanup_push(cleanup, "thread5 second handler");
    if (arg)
        return ((void*)1);
    pthread_cleanup_pop(0); // 不会调用
    pthread_cleanup_pop(0);

    return ((void*)1);
}

void *thread_fun6(void *arg)
{
    printf("thread6 start\n");
    pthread_cleanup_push(cleanup, "thread6 first handler");
    pthread_cleanup_push(cleanup, "thread6 second handler");
    if (arg)
        pthread_exit((void*)2);
    pthread_cleanup_pop(0); // 不会调用
    pthread_cleanup_pop(0);

    pthread_exit((void*)2);
}

void thread_cleanup_test1(void)
{
    int err;
    pthread_t tid5, tid6;
    void *tret;

    err = pthread_create(&tid5, NULL, thread_fun5, (void*)1);
    if (err != 0) {
        err_ret("can't create thread5");
    }
    err = pthread_create(&tid6, NULL, thread_fun6, (void*)1);
    if (err != 0) {
        err_ret("can't create thread6");
    }

    err = pthread_join(tid5, &tret);
    if (err != 0) {
        err_ret("Can't join with thread5");
    }
    printf("thread5 exit with code:%ld\n", (long)tret);
    err = pthread_join(tid6, &tret);
    if (err != 0) {
        err_ret("Can't join with thread6");
    }
    printf("thread6 exit with code:%ld\n", (long)tret);
    exit(0);
}

void cleanup(void *arg)
{
    printf("cleanup arg:%s\n", (char*)arg);
}

// 线程同步
// 当多个线程共享同一内存时, 需要确保每一个线程看到一致的数据视图
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER; // 静态内存
pthread_mutex_t *pMutex2 = NULL; // 动态分配内存

void thread_sync_test1(void)
{

}


// 避免死锁
// 1. 同一个锁加锁两次;
// 2. 两个锁, 不同线程使用的顺序相反时也会导致死锁
// 解决措施:
// 1. 仔细检查上锁顺序, 确保不会出现相反顺序上锁情况;
// 2. 需要对多个锁进行上锁时, 可以使用 pthread_mutex_trylock() 获取成功, 前进;失败则释放之前的锁, 一段时间后重试
static pthread_mutex_t mutex8 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex9 = PTHREAD_MUTEX_INITIALIZER;
static uint32_t test_cnt8 = 0;
pthread_t tid8, tid9;
void *thread9_entry(void *arg);

void thread_lock_test2(void)
{
    int ret;

    ret = pthread_create(&tid8, NULL, thread9_entry, "thread8");
    if (ret != 0) {
        err_ret("create thread8 failed\n");
    }
    ret = pthread_create(&tid9, NULL, thread9_entry, "thread9");
    if (ret != 0) {
        err_ret("create thread9 failed\n");
    }

    pause();
}

void *thread9_entry(void *arg)
{
    while(1) {
        pthread_mutex_lock(&mutex8);
        pthread_mutex_lock(&mutex9);
        printf("%s:%u\n", (char*)arg, ++test_cnt8);
        // pthread_mutex_lock(&mutex9); // 死锁:重复上锁
        pthread_mutex_unlock(&mutex9);
        pthread_mutex_unlock(&mutex8);
    }
}

void thread_lock_test3(void)
{
    int ret;
    struct timespec timeout;
    struct tm *tmp;
    char buf[64];
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&lock);
    printf("mutex is locked\n");
    clock_gettime(CLOCK_REALTIME, &timeout);
    tmp = localtime(&timeout.tv_sec);
    strftime(buf, sizeof(buf), "%I:%M:%S %p.", tmp);
    printf("current time:%s\n", buf);

    // pthread_mutex_timed_lock 要使用 GMT 时间, 从当前时间开始阻塞 10s
    timeout.tv_sec += 10; 
    // 这里会导致死锁
    ret = pthread_mutex_timedlock(&lock, &timeout);
    clock_gettime(CLOCK_REALTIME, &timeout);
    tmp = localtime(&timeout.tv_sec);
    strftime(buf, sizeof(buf), "%r", tmp);
    printf("This time now is:%s\n", buf);
    if (ret == 0) {
        printf("locked again\n");
    } else {
        printf("cant' lock again errno:%d %s\n", errno, strerror(ret));
    }
}

void sysconf_test2(void)
{
    printf("PTHREAD_DESTRUCTOR_ITERATIONS:%ld\n", sysconf(_SC_THREAD_DESTRUCTOR_ITERATIONS));
    printf("PTHREAD_KEYS_MAX:%ld\n", sysconf(_SC_THREAD_KEYS_MAX));
    printf("PTHREAD_STACK_MIN:%ld\n", sysconf(_SC_THREAD_STACK_MIN));
    printf("PTHREAD_THREADS_MAX:%ld\n", sysconf(_SC_THREAD_THREADS_MAX));
}

int makethread(void *(*fun)(void *), void *arg)
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

// 线程属性的 get/set 方法
void thread_property_test1(void)
{
    int ret;
    pthread_t tid;
    pthread_attr_t attr;
    int state;
    void *stackaddr;
    size_t statcksize;
    long size;
    void *thread_ret;
    uint8_t *real_stack_addr; // 这申请的内存什么时候释放?
    size_t real_stack_size;

    ret = pthread_attr_init(&attr);
    ASSERT(ret == 0); // if (ret != 0) {printf("assert %s\n", "ret != 0");}

    // ret = pthread_attr_getdetachstate(&attr, &state);
    // ASSERT(ret == 0);
    // printf("before state:%d\n", state);
    // ret = pthread_attr_setdetachstate(&attr, 1);
    // ASSERT(ret == 0);
    // ret = pthread_attr_getdetachstate(&attr, &state);
    // ASSERT(ret == 0);
    // printf("after state:%d\n", state);

    ret = pthread_attr_getguardsize(&attr, &statcksize); // 4K
    ASSERT(ret == 0);
    printf("default thread guard size:%lu\n", statcksize);

#if defined(_POSIX_THREAD_ATTR_STACKADDR) && defined(_POSIX_THREAD_ATTR_STACKSIZE)
    // 先查看操作系统是否支持线程栈属性
    size = sysconf(_SC_THREAD_ATTR_STACKADDR);
    printf("thread stack:%lu\n", size);
    size = sysconf(_SC_THREAD_ATTR_STACKSIZE);
    printf("thread stack size:%lu\n", size);
    if (size) {
        ret = pthread_attr_getstack(&attr, &stackaddr, &statcksize);
        ASSERT(ret == 0);
        printf("thread stack addr:%p size:%lu\n", stackaddr, statcksize);
        real_stack_addr = (uint8_t*)malloc(20 * KB);
        ASSERT(real_stack_size);
        real_stack_size = 20 * KB;
        ret = pthread_attr_setstack(&attr, real_stack_addr, real_stack_size);
        // printf("ret:%d errno:%d\n", ret, errno);        
        ASSERT(ret == 0);
        printf("set thread stack addr:%p size:%lu minimum size:%d\n", 
            real_stack_addr, real_stack_size, PTHREAD_STACK_MIN);
        ret = pthread_attr_getstack(&attr, &stackaddr, &statcksize);
        ASSERT(ret == 0);
        printf("thread stack addr:%p size:%lu\n", stackaddr, statcksize);
    }
#else
#warning "set stack not support"
#endif


    ret = pthread_create(&tid, &attr, thread_fun3, NULL);
    ASSERT(ret == 0);
    ret = pthread_join(tid, &thread_ret);
    ASSERT(ret == 0);
    
    pthread_attr_destroy(&attr);
    free(real_stack_addr); // 这内存在哪里释放

    return;
}

#define MAXSTRINGSZ         4096
static char envbuf_m[MAXSTRINGSZ];
extern char **environ; // 环境变量

// getenv 可能的一种实现(线程不安全)
char *getenv_m(const char *name)
{
    int i, len;

    len = strlen(name);
    for (i = 0; environ[i] != NULL; i++) {
        if ((strncmp(name, (char*)environ[i], len) == 0) && environ[i][len] == '=') {
            strncpy(envbuf_m, (char*)&environ[i][len+1], MAXSTRINGSZ - 1);
            return (envbuf_m);
        } 
    }

    return (NULL);
}

static pthread_mutex_t mutex10;
static pthread_once_t init_done = PTHREAD_ONCE_INIT; // 每个进程只调用一次
static void thread_init(void);
// 线程安全的实现, 需要自己提供内存, 而不是使用库公共的内存
int getenv_r_m(const char *name, char *buf, int buflen)
{
    int i, len, olen;

    pthread_once(&init_done, thread_init); // pthread_once
    len = strlen(name);
    pthread_mutex_lock(&mutex10);
    for (i = 0; environ[i] != NULL; i++) {
        if ((strncmp(name, (char*)environ[i], len) == 0) && environ[i][len] == '=') {
            olen = strlen(&environ[i][len+1]);
            if (olen > buflen) {
                pthread_mutex_unlock(&mutex10);
                return (ENOSPC);
            }
            strncpy(buf, &environ[i][len], buflen);
            pthread_mutex_unlock(&mutex10);
            return (0);
        }
    }
    pthread_mutex_unlock(&mutex10);

    return (ENOENT);
}

static void thread_init(void)
{
    int ret;
    pthread_mutexattr_t attr;

    ret = pthread_mutexattr_init(&attr);
    ASSERT(ret == 0);
    ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ASSERT(ret == 0);
    ret = pthread_mutex_init(&mutex10, &attr);
    ASSERT(ret == 0);
    ret = pthread_mutexattr_destroy(&attr);
    ASSERT(ret == 0);
}

void getenv_test(void)
{
    char buf[512] = {0};

    printf("path:%s\n", getenv_m("PATH"));
    getenv_r_m("PATH", buf, sizeof(buf));
    printf("path:%s\n", buf);
}

// 线程特定数据/线程私有数据
void destructor(void *arg) {}
pthread_key_t key;
int init_done2 = 0;

void *thread_fun11(void *arg)
{
    int ret;
    if (!init_done2) { // 初始化阶段存在竞争而出现多次创建 key 的问题, 引入 pthread_once()
        init_done2 = 1;
        ret = pthread_key_create(&key, destructor);
    }
}

void thread_test11(void)
{
    int ret;
    pthread_t tid;

    for (int ix = 0; ix < 10; ++ix) {
       ret = pthread_create(&tid, NULL, thread_fun11, NULL);
       ASSERT(ret == 0);
    }
}

pthread_once_t initflag = PTHREAD_ONCE_INIT;
void thread_init12(void)
{
    int ret;

    ret = pthread_key_create(&key, destructor);
    ASSERT(ret == 0);

    return;
}

void thread_test12(void)
{
    pthread_once(&initflag, thread_init12);
    // 后续其他操作
}

pthread_mutex_t lock12 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock13 = PTHREAD_MUTEX_INITIALIZER;
void prepare(void);
void parent(void);
void child(void);
void *thread_fun12(void *arg);

void fork_handler_test1(void)
{
    int err;
    pid_t pid;
    pthread_t tid;

    if ((err = pthread_atfork(prepare, parent, child)) != 0)
        err_quit("failed to install handler");
    err = pthread_create(&tid, NULL, thread_fun12, NULL);
    ASSERT(err == 0);

    sleep(2);
    printf("prepare to fork\n");

    if ((pid = fork()) < 0) {
        err_ret("failed");
    } else if (pid == 0) {
        printf("child:%d parentid:%d return from fork\n", getpid(), getppid());
    } else {
        printf("prent:%d return from fork\n", getpid());
    }
    exit(0);
}
 
void *thread_fun12(void *arg)
{
    printf("thread started\n");
    pause();
    return (0);
}

void prepare(void)
{
    int err;

    printf("prepare locking\n");
    if ((err = pthread_mutex_lock(&lock12)) != 0)
        err_ret("lock failed")
    if ((err = pthread_mutex_lock(&lock13)) != 0)
        err_ret("lock failed")
}

void parent(void)
{
    int err;

    printf("parent unlocking\n");
    if ((err = pthread_mutex_unlock(&lock12)) != 0)
        err_ret("lock failed")
    if ((err = pthread_mutex_unlock(&lock13)) != 0)
        err_ret("lock failed")
}

void child(void)
{
    int err;

    printf("child unlocking\n");
    if ((err = pthread_mutex_unlock(&lock12)) != 0)
        err_ret("lock failed")
    if ((err = pthread_mutex_unlock(&lock13)) != 0)
        err_ret("lock failed")
}