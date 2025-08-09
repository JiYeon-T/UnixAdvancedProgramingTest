#include "../common/apue.h"
#include "pthread.h"

// 增大锁的粒度(代码可读性增强,复杂性降低,线程之间的阻塞增多)
// 在代码复杂性和性能之间找到平衡

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

        // pthread_mutex_unlock(&hashlock);
        // pthread_mutex_unlock(&fp->f_lock);
    }

    return (fp);
}

void foo_hold(struct foo *fp)
{
    pthread_mutex_lock(&hashlock);
    fp->f_count++;
    pthread_mutex_unlock(&hashlock);
}

struct foo *foo_find(int id)
{
    struct foo *fp = NULL;

    pthread_mutex_lock(&hashlock);
    for (fp = fh[HASH(id)]; fp != NULL; fp = fp->f_next) {
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

    pthread_mutex_lock(&hashlock); // 增大 hashlock 粒度
    if (--fp->f_count == 0) {
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
        pthread_mutex_destroy(&fp->f_lock);
        free(fp);
    } else {
        pthread_mutex_unlock(&hashlock);
    }
}

void lock_test()
{
}