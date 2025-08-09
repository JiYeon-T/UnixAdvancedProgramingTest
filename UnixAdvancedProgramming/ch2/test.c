#include "test.h"
#include <stdio.h>
#include <stdlib.h>

#include <limits.h> // PATH_MAX...
#include <error.h>
#include <errno.h>


// #include <confname.h>


/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h> // 移植有关的编译选项都放在这个头文件中, 如 PATH_MAX, NAME_MAX...

#include <sys/types.h> // 基本数据类型

#include <sys/param.h>

#include <time.h> // ISO clock()

/*
 *	POSIX Standard: 4.5.2 Process Times	<sys/times.h>
 */
#include <sys/times.h> // POSIX

/*
 *	POSIX Standard: 6.5 File Control Operations	<fcntl.h>
 */
#include <fcntl.h>

/*************
 * MACRO 
 *************/
#define UNUSED_PARAM(p)       ((void)p)

#define err_quit(format, ...) printf(format", log output.\n", ##__VA_ARGS__)
#define err_sys(format, ...)  printf(format", log output.\n", ##__VA_ARGS__)
#define err_ret(format, ...)  printf(format", log output.\n", ##__VA_ARGS__)

#define BUFFERSIZE            4096
#define MAXLINE               1


#ifdef PATH_MAX
static long pathmax = PATH_MAX;
#else
static long pathmax = 0;
#endif

#define PATH_MAX_GUESS          1024

#ifdef OPEN_MAX
static long openmax = OPEN_MAX;
#else
static long openmax = 0;
#endif

#define OPEN_MAX_GUESS          256

#define USER_TIME_TEST          0
#define SYSTEM_TIME_TEST        0
#define CHILD_TIME_TEST         1


/*************
 * VARIABLE 
 *************/
static long posix_version = 0;
static long xsi_version;



/**
 * @fun: get system config
 *       long sysconf(int name);
 *       long pathconf(const char *path, int name);
 *       long fpathconf(int fd, int name);
 *       有些参数并不是设置多少就是多少的, 是会根据系统动态变化的所以需要这三个系统 API
*/
void get_config(void)
{
    long val;
    int fd;
    long version;

    // ARG_MAX 在 #include <limits.h> 中有定义, 但是仍然后可能变化
    val = sysconf(_SC_ARG_MAX);
    if(val == -1) {
        err_sys("get config failed");
        err_sys("errno:%d", errno);
    }
    printf("ARG_MAX:%ld\r\n", val);

    val = sysconf(_SC_ATEXIT_MAX);
    if(val == -1) {
        err_sys("get config failed");
        err_sys("errno:%d", errno);
    }
    printf("_SC_ATEXIT_MAX:%ld\r\n", val);

    version = sysconf(_SC_VERSION);
    if(val == -1) {
        err_sys("get config failed");
        err_sys("errno:%d", errno);
    }
    printf("POSIX VERSION:%ld\r\n", version);

    val = sysconf(_SC_CLK_TCK);
    if(val == -1) {
        err_sys("get config failed");
        err_sys("errno:%d", errno);
    }
    printf("TICK COUNT per second:%ld\r\n", val);

    val = pathconf(".", _PC_NAME_MAX);
    if(val == -1) {
        err_sys("get pathconfig failed");
        err_sys("errno:%d", errno);
    }
    printf("NAME_MAX:%ld\r\n", val);

    val = pathconf(".", _PC_PATH_MAX);
    if(val == -1) {
        err_sys("get pathconfig failed");
        err_sys("errno:%d", errno);
    }
    printf("PATH_MAX:%ld\r\n", val);

    fd = open("./test.txt", O_RDONLY);
    if(fd < 0) {
        err_sys("open file failed");
        return;
    }
    printf("file fd:%d\r\n", fd);

    val = fpathconf(fd, NAME_MAX);
    if(val == -1) {
        err_sys("get fpathconfig failed");
        err_sys("errno:%d", errno);
    }
    printf("fpathconfig PATH_MAX:%ld\r\n", val);


    close(fd);
    exit(0);
}

/**
 * @fun: 
*/
char* path_alloc(size_t *sizep)
{
    char *ptr;
    size_t size;
    char path[2048];
    char *pRet;

    if(!sizep) {
        err_sys("null parameter");
        return NULL;
    }

    if(posix_version == 0) {
        posix_version = sysconf(_SC_VERSION);
    }

    if(xsi_version == 0) {
        xsi_version = sysconf(_SC_XOPEN_VERSION);
    }
    printf("posix_version:%ld, xsi_version:%ld\r\n", posix_version, xsi_version);


    if(pathmax == 0) {
        errno = 0;
        if((pathmax = pathconf("/", _PC_PATH_MAX)) < 0) {
            if(errno == 0) {
                pathmax = PATH_MAX_GUESS;
            }
            else {
                err_sys("get pathconfig error for _PC_PATH_MAX");
            }
        } else {
            pathmax++;
        }
    }

    // 兼容不同的 POSIX 标准
    if((posix_version < 200112L) && (xsi_version < 4))
        size = pathmax + 1;
    else
        size = pathmax;

    if((ptr = malloc(size)) == NULL) {
        err_sys("malloc failed");
        return NULL;
    }

    *sizep = size;

    printf("path len:%lu\r\n", size);
    printf("standard path len:%d\r\n", MAXPATHLEN);

    pRet = getcwd(path, 2048);
    if(!pRet) {
        err_sys("getcwd error");
        return NULL;
    }
    printf("current path:%s\r\n", path);

    return (ptr);
}

/**
 * @fun:close all opened file method
 * 
*/
void file_op_test()
{
    // 守护进程中一个常见的代码序列是关闭所有打开文件
    for(int i = 0; i < NOFILE; ++i) {
        printf("close %d\r\n", i);
        close(i);
    }

    // #include <stdio.h>
    // for(int i = 0; i < _NFILE; ++i) {
    //     close(i);
    // }

    // POSIX.1  OPEN_MAX
    for(int i = 0; i < sysconf(_SC_OPEN_MAX); ++i) {
        close(i);
    }

    // 
}

/**
 * @fun:守护进程中一个常见的代码序列是关闭所有打开文件
*/
long int daemon_process_test()
{
    if(openmax == 0) {
        errno = 0;
    }

    if((openmax = sysconf(_SC_OPEN_MAX)) < 0) {
        if(errno == 0) {
            openmax = OPEN_MAX_GUESS;
        } else {
            err_sys("open err _SC_OPEN_MAX");
        }
    }
    printf("openmax:%ld\r\n", openmax);

    return (openmax);
}



/**
 * @fun: ISO 标准与 POSIX 标准可能出现冲突, eg: clock
*/
void clock_test()
{
    // clock_t startClock;

    // startClock = clock(); // ISO 标准, 没有定义单位, 换算成秒需要 / CLOCKS_PER_SEC
    // printf("startClock:%ld %ld", (long)clock, clock / CLOCKS_PER_SEC);

    // POSIX.1 定义了 times 函数, 
    // startClock = times(); // POSIX 使用 times() 获取终止子线程的运行时间以及时钟时间, 也是 clock_t 类型
}

/**
 * @fun:
 *      clock_t times(struct tms *buf); // get process  time
*/
void times_test()
{
    // 系统每秒的时钟数可以通过 sysconf(_SC_CLK_TCK) 获取
    // 其中 _SC_CLK_TCK 为 2, /usr/include/bits/confname.h 
    long tickPerSec;
    struct tms tmsStart, tmsEnd;
    clock_t clockHead, clockEnd;

    tickPerSec = sysconf(_SC_CLK_TCK);
    printf("tickPerSec:%ld\r\n", tickPerSec);
    
    clockHead = times(&tmsStart); // 进程运行到此时的系统时钟数
    printf("clockStart:%f\r\n", clockHead / (double)tickPerSec);

    // Execute the given line as a shell command.
    // system("./time_test.exe");
    // system("sleep 2"); // 休眠 2s, shell: sleep 2
#if USER_TIME_TEST
    for(int ix = 0; ix < 20000; ++ix) {
        for(int jx = 0; jx < 20000; ++jx)
            ;
    }
#endif

#if SYSTEM_TIME_TEST
    for(int ix = 0; ix < 1000; ++ix) {
        for(int jx = 0; jx < 1000; ++jx) {
            open("CannotOpen.txt", O_RDONLY);
        }
    }
#endif

#if CHILD_TIME_TEST
    system("./time_test.exe");
#endif
    clockEnd = times(&tmsEnd); // 进程运行到此时的系统时钟数

    // clock_t -> second
    printf("clockEnd:%f\r\n", clockEnd / (double)tickPerSec);

    // 打印用户时间, 系统 CPU 时间, 子进程用户时间, 子进程系统 CPU 时间
    printf("user time:%f\n", (tmsEnd.tms_utime - tmsStart.tms_utime) / (double)tickPerSec);
    printf("system time:%f\n", (tmsEnd.tms_stime - tmsStart.tms_stime) / (double)tickPerSec);
    printf("child user time:%f\n", (tmsEnd.tms_cutime - tmsStart.tms_cutime) / (double)tickPerSec);
    printf("child user time:%f\n", (tmsEnd.tms_cstime - tmsStart.tms_cstime) / (double)tickPerSec);

    exit(0);
}
