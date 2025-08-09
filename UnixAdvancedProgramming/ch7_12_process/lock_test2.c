#include "../common/apue.h"
#include "pthread.h"

// 两个锁同时使用, 控制上锁顺序的方法
// 使用了散列表:????????????????


#define NHASH           29
#define HASH(id)        (((unsigned long)id) % NHASH)

struct foo;

struct foo *fh[NHASH];

struct foo {
    int f_count;
    pthread_mutex_t f_lock;
    int f_id;
    struct foo *f_next; // protected by hash lock
    // more stuff here
};

pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;


struct foo *foo_alloc(int id) // allocate the object
{
    struct foo *fp = NULL;
    int idx;

    if ((fp = (struct foo*)malloc(sizeof(struct foo))) != NULL) {
        fp->f_count = 1;
        fp->f_id = id;
        if (pthread_mutex_init(&fp->f_lock, NULL) != 0) {
            free(fp);
            return (NULL);
        }
        idx = HASH(id);
        pthread_mutex_lock(&hashlock);
        fp->f_next = fh[idx]; // pointer type
        fh[idx] = fp;
        pthread_mutex_lock(&fp->f_lock); // fp->lock
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_unlock(&hashlock);
        // TODO:两种不同的释放锁的顺序有什么区别?
        // pthread_mutex_unlock(&hashlock);
        // pthread_mutex_unlock(&fp->f_lock);
    }

    return (fp);
}

void foo_hold(struct foo *fp)
{
    pthread_mutex_lock(&fp->f_lock);
    fp->f_count++;
    pthread_mutex_unlock(&fp->f_lock);
}

struct foo *foo_find(int id)
{
    struct foo *fp = NULL;

    pthread_mutex_lock(&hashlock);
    for (fp = fh[HASH(id); fp != NULL; fp = fp->f_next]) {
        if (fp->f_id == id) {
            foo_hold(&fp->f_lock);
            break;
        }
    }
    pthread_mutex_unlock(&hashlock);

    return (fp);
}

void foo_release(struct foo *fp)
{
    struct foo *tfp;
    int idx;

    pthread_mutex_lock(&fp->f_lock);
    if (fp->f_count == 1) {
        pthread_mutex_unlock(&fp->f_lock); // 这种写法是为了控制锁的粒度??

        pthread_mutex_lock(&hashlock);
        pthread_mutex_lock(&fp->f_lock);
        // need to recheck the condition
        if (fp->f_count != 1) {
            pthread_mutex_unlock(&fp->f_lock);
            pthread_mutex_unlock(&hashlock);
            return;
        }
        // f_count==1, remove from list, release
        // TODO: 不明白:
        // 1. Hash
        // 2. 这儿的这个链表感觉有些奇怪
        idx = HASH(fp->f_id);
        tfp = fh[idx];
        if (tfp == fp) {
            fh[idx] = fp->f_next;
        } else {
            while (tfp->f_next != fp)
                tfp = tfp->f_next;
            tfp->f_next = fp->f_next;
        }
        pthread_mutex_unlock(&hashlock);
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_destroy(&fp->f_lock);
        free(fp);
    } else {
        fp->f_count--;
        pthread_mutex_unlock(&fp->f_lock);
    }
}

void lock_test()
{

}