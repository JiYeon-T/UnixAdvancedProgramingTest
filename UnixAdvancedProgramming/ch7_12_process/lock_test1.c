#include "../common/apue.h"
#include "pthread.h"


// 应用场景:当使用动态分配的内存时, 要确保在最后一个线程访问该内存前, 动态分配的内存没有被释放掉
// 确保每一个线程去访问结构体的时候, 这块内存都还是在的, 正常使用的
// 通常的解决措施是:使用 cnt (计数变量)来统计动态内存被引用的次数

struct foo {
    int f_count;
    pthread_mutex_t f_lock;
    int f_id;
    /* more stuff here */
};

// alloc the object
struct foo * foo_alloc(int id)
{
    struct foo *fp = NULL;

    if ((fp = (struct foo*)malloc(sizeof(struct foo))) != NULL) {
        fp->f_count = 1;
        fp->f_id = id;
        if (pthread_mutex_init(&fp->f_lock, NULL) != 0) {
            free(fp);
            return (NULL);
        }
    }

    return (fp);
}

void foo_hold(struct foo *fp)
{
    pthread_mutex_lock(&fp->f_lock);
    ++fp->f_count;
    pthread_mutex_unlock(&fp->f_lock);
}

void foo_release(struct foo *fp)
{
    pthread_mutex_lock(&fp->f_lock);
    if (--fp->f_count == 0) { // last reference
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_destroy(&fp->f_lock);
        free(fp);
    } else {
        pthread_mutex_unlock(&fp->f_lock);   
    }
}

static void *thread_entry(void *arg);
pthread_t thread1, thread2;
static struct foo *fp = NULL;

void lock_test1(void)
{
    int ret;
    fp = foo_alloc(1);

    if (!fp) {
        exit(-1);
    }

    ret = pthread_create(&thread1, NULL, thread_entry, "thread1");
    if (ret != 0) {
        err_ret("create thread1 failed\n");
    }
    ret = pthread_create(&thread2, NULL, thread_entry, "thread2");
    if (ret != 0) {
        err_ret("create thread2 failed\n");
    }
    pause();

}

static void *thread_entry(void *arg)
{
    printf("%s enter\n", (char*)arg);

    while (1) {
        foo_hold(fp);
        printf("%s id:%d cnt:%d\n", (char*)arg, fp->f_id, fp->f_count);
        foo_release(fp);
        sleep(1);
    }
    printf("%s exit\n", (char*)arg);
}
