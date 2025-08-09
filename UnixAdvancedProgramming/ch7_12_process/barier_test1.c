#include "../common/apue.h"
#include <pthread.h>
#include <limits.h>
#include <sys/time.h>


// 使用屏障进行线程同步, 使用多线程对 800万个数进行排序
// TODO:

#define NTHR        (8)
#define NUMNUM      (800000L)
#define TNUM        (NUMNUM / NTHR)

long nums[NUMNUM];
long snums[NUMNUM];

static pthread_barrier_t barrier;

// #ifdef SOLARIS
// #define heapsort qsort
// #else
// extern int heapsort(void *, size_t, size_t, int (*)(const void *, const void *));
// #endif

// 冒泡排序
int heapsort(void *p_arr, size_t len, size_t itemSize, int (*cmp)(const void *, const void *))
{
    size_t ix, jx;
    long num;
    long *p = (long*)p_arr;

    for (ix = 0; ix < len - 1; ++ix) {
        for (jx = ix + 1; jx < len; ++jx) {
            if (cmp(&p[ix], &p[jx]) > 0) { // 从小到达排序
                num = p[ix]; // 交换
                p[ix] = p[jx];
                p[jx] = num;
            }
        }
    }
}

// 供 heapsort 使用的函数指针
int complong(const void *arg1, const void *arg2)
{
    long l1 = *(long*)arg1;
    long l2 = *(long*)arg2;

    if (l1 == l2)
        return 0;
    else if (l1 > l2)
        return 1;
    else
        return -1;
}

void *thread_fun(void *arg)
{
    long idx = (long)arg;

    heapsort(&nums[idx], TNUM, sizeof(long), complong);
    pthread_barrier_wait(&barrier); // wait other thread done
    
    // get off and perform more work

    return (0);
}

// 合并 8 个有序数组
void merge(void)
{
    long idx[NTHR]; // 保存每个数组的开始下标
    long i, min_idx, start_idx, min_num;

    for (i = 0; i < NTHR; ++i)
        idx[i] = i * TNUM;
    
    for (start_idx = 0; start_idx < NUMNUM; ++start_idx) {
        min_num = LONG_MAX;
        for (i = 0; i < NTHR; ++i) { // 找 8 个数组第一个元素 中的最小值保存到目标数组中
            if ((idx[i] < (i + 1) * TNUM) && (nums[idx[i]] < min_num)) {
                min_num = nums[idx[i]];
                min_idx = i;
            }
        }
        // 保存 arr0[0], arr1[0], arr2[0]... arr7[0] 中的最小值
        snums[start_idx] = nums[idx[min_idx]];
        idx[min_idx]++; // 用了哪个数组的最小值, 哪个数组索引就 +1
    }
}

void barrier_test(void)
{
    unsigned long i;
    struct timeval start, end;
    long long startusec, endusec;
    double elapsed;
    int err;
    pthread_t tid;
    
    srandom(1);
    for (i = 0; i < NUMNUM; ++i)
        nums[i] = random();
    
    // create threads
    pthread_barrier_init(&barrier, NULL, NTHR + 1); // 包括主线程
    gettimeofday(&start, NULL);
    for (i = 0; i < NTHR; ++i) {
        // 传入参数为数组索引
        err = pthread_create(&tid, NULL, thread_fun, (void*)(i * TNUM));
        if (err != 0)
            err_ret("create thread failed");
    }
    //TODO:
    // 主线程子线程同时阻塞等待, 同时结束
    pthread_barrier_wait(&barrier);
    merge();
    gettimeofday(&end, NULL);

    startusec = start.tv_sec * 1000000 + start.tv_usec;
    endusec = end.tv_sec * 1000000 + end.tv_usec;
    // printf("%lld %lld\n", startusec, endusec);
    elapsed = (double)(endusec - startusec) / 1000000.0;
    printf("sort use %.4f seconds\n", elapsed);

    exit(0);
}