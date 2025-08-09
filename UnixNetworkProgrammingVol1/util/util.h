#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#include <unistd.h>


// #include <bluetooth.h> // TODO:bluetooth programing at UNIX, hciattach  hciconfig  hcitool    


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/stropts.h> // INTTIM
#include <sys/errno.h>
#include <signal.h>
#include <time.h>
/***************************************************************
 * MACRO
 ***************************************************************/
#define UNUSED_PARAM(p)       ((void)p)

#define err_quit(format, ...) { printf(format"\r\n", ##__VA_ARGS__); \
                                exit(-1);                            \
                               }
#define err_exit(format, ...) { printf(format"\r\n", ##__VA_ARGS__); \
                                exit(-1);                            \
                               }
#define err_sys(format, ...)  ( printf(format"\r\n", ##__VA_ARGS__))
#define err_ret(format, ...)  { printf(format"\r\n", ##__VA_ARGS__); \
                                return;                              \
                               }
#define err_dump(format, ...)  { printf(format"\r\n", ##__VA_ARGS__); \
                                 exit(-1);                            \
                                }

// 仅仅测试使用, 正式代码中尽量不要用 assert
#define ASSERT(cond) {                                      \
    if (!(cond)) {                                          \
        printf("assert %s line:%d\n", #cond, __LINE__);     \
        exit(-1);                                           \
    }                                                       \
}

#define KB                       (1024)
#define MB                       (1024 * 1024)
#define GB                       (1024 * 1024 * 1024)

#define BUFFERSIZE                (4096)
#define MAXLINE                   (512)

#define FILE_MODE                 (0666)

#define IPV4_TEST                   1
// #define IPV6_TEST                   1

#define LISTENQ                     (100)
#define OPEN_MAX                    (1024)
#define INFTIM                      (-1) // TODO: where defined this??

// function time usage debug
#define TIME_MEASURE_START()                        \
    struct timespec ts1, ts2;                       \
    clock_gettime(CLOCK_MONOTONIC, &ts1);

// long type
#define TIME_MEASURE_END(ll_ns)                     \
    clock_gettime(CLOCK_MONOTONIC, &ts2);           \
    ll_ns = (ts2.tv_sec * 1000000000 + ts2.tv_nsec) -  \
        (ts1.tv_sec * 1000000000 + ts1.tv_nsec);


#define LOG_D(format,...)           printf("[D] "format"\r\n", ##__VA_ARGS__)
#define LOG_I(format,...)           printf("[I] "format"\r\n", ##__VA_ARGS__)
#define LOG_W(format,...)           printf("[W] "format"\r\n", ##__VA_ARGS__)
#define LOG_E(format,...)           printf("[E] "format"\r\n", ##__VA_ARGS__)


int max(int a, int b);

void *Malloc(size_t size);
void Free(void *ptr);
void *Calloc(size_t nmemb, size_t size);
void *Realloc(void *ptr, size_t size);


#endif /* __UTIL_H__ */
