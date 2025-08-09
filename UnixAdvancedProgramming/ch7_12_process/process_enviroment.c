#include "../common/apue.h"

#include <stdio.h>
#include <stdlib.h>

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/*
 *	ISO C99 Standard: 7.13 Nonlocal jumps	<setjmp.h>
 */
#include <setjmp.h>

#include <sys/resource.h>
/*****************************************************************************
 * DELCARATIONS
 *****************************************************************************/
// 进程终止
// 正常终止:5 种方法: a. exit(); b._exit(); c. _Exit(); d.最后一个线程正常退出; e.最后一个线程调用 pthread_exit() 退出;
//异常终止:a. abort(), raise SIGABRT signal; b.signal; c. lasht thread receive pthread_cancel()'s signal;
void exit(int status); // status 进程退出的终止状态; 退出时关闭标准 IO
void _Exit(int status);
void _exit(int status);
int atexit(void (*func)(void));
// memory
void *malloc(size_t size);
void *calloc(size_t nobj, size_t size);
void *realloc(void *ptr, size_t newsize);
void free(void *ptr);
// enviroment variable - environ 变量
char *getenv(const char *name);
int putenv(char *str);
int setenv(const char *name, const char *value, int rewrite); // 设置全局变量 environ
int unsetenv(const char *name);
int clearenv(void);

// Nonlocal goto, 非局部 goto, 跳过几个调用栈, 然后返回到调用当前函数的某一个栈帧中
//TODO:
// 中间跳过的栈帧如何处理???
int setjump(jmp_buf env);
void longjmp(jmp_buf env, int val);
// process resource
int getrlimit(int resource, struct rlimit *rlptr);
int setrlimit(int resource, const struct rlimit *rlptr);
/*****************************************************************************
 * PUBLIC FUNCTIONS
 *****************************************************************************/
static void my_exit1(void);
static void my_exit2(void);

/**
 * @brief 注册线程终止处理程序 (exit handler 按照栈的格式存放)
*/
void exit_test(int argc, char *argv[])
{
    // 不同操作系统可以注册的 exit handler 数目有限, 取决于具体实现, 获取方法:
    fprintf(stdout, "max atexit num:%ld\n", sysconf(_SC_ATEXIT_MAX));

    // exit() 调用顺序与注册顺序相反: 栈
    if (atexit(my_exit1) != 0) {
        err_ret("Can't register exit handler");
    }
    if (atexit(my_exit2) != 0) {
        err_ret("Can't register exit handler");
    }

    // 命令行参数, NULL 结尾
    for (int ix = 0; argv[ix] != NULL; ++ix) {
        printf("argv[%d]:%s\n", ix, argv[ix]);
    }
}

static void my_exit1(void)
{
    printf("first exit handler:%s\n", __func__);
}

static void my_exit2(void)
{
    printf("second exit handler:%s\n", __func__);
}

/**
 * @brief 环境变量保存在环境表(数据结构:"name=value"结构的指针数组)中, 全局变量 environ 指向该环境表
 * 
*/
void env_test()
{
    char *key = "PATH";
    char *value;
    extern char **environ; // std 提供的环境变量指针数组
    char **p_environ = environ;
    int ix = 0;

    exit(-1);

    if (!p_environ) {
        fprintf(stderr, "get environ failed");
        exit(-1);
    }
    // 遍历 environ, 打印出所有的环境变量
    while (*p_environ) {
        fprintf(stdout, "environ[%d]:%s\n", ix++, *p_environ++);
    }

    if ((value = getenv(key)) == NULL) {
        err_ret("getenv failed");
    }
    printf("%s:%s\n", key, value);

    if (putenv("TEST_ENV_PATH=/user/include") != 0) {
        err_ret("putenv failed");
    }

    // check
    if ((value = getenv("TEST_ENV_PATH")) == NULL) {
        err_ret("getenv failed");
    }
    printf("TEST_ENV_PATH:%s\n", value);

    key = (char*)malloc(200);
    strcpy(key, "TEST_EVN_PATH2");

    value = (char*)malloc(200);
    strcpy(value, "/user/include2");

    if (setenv(key, value, 0) != 0) {
        err_ret("set env failed");
    }

    memset(value, 0, 200);

    if ((value = getenv(key)) == NULL) {
        err_ret("getenv failed");
    }
    printf("%s:%s\n", key, value);  

    //TODO:
    //必现 crash:这内存被添加到环境变量表使用, 不可以释放??
    // free(key);
    // free(value);

    return;
}

/**
 * @brief C程序的存储空间分布(本机为:64 位 Intel x86 处理器上的 Linux) 
 * 1.未初始化数据段的内容并不存放在程序文件中. 其原因是,内核在开始运行前将他们都设为 0
 * 需要存放在程序文件中的只有 正文段(.text)和已初始化数据段(.data)
 *
 * 
 * 高地址:
 * ------------------ |
 * 命令行参数和环境变量  |
 * ------------------ |
 * 
 * 栈(向下增长)
 *
 * ------------------ |
 * 
 * 堆(向上增长)
 * 
 * ------------------ |
 * 未初始化的数据(.bss) |
 * ------------------ |
 * 初始化的数据(.data)  |
 * ------------------ |
 * 正文/代码段(.text)   |
 * -------------------|
 * 低地址:
*/
static int uninitialize_var1;
static int initialized_var1 = 100;
static void this_is_a_fun(void) {}
static void print_fun_addr(void (*p_fun)(void))
{
    printf("p_fun address:%p\n\n", p_fun);
}
void process_memory_area_distributin(int argc, char *argv[])
{
    // 对于 32 位 Intel x86 处理器上的 Linux, 
    // 正文段从 0x08048000 单元开始
    // 栈底从:0xC0000000 开始, 在这种结构中, 栈从高地址向低地址方向增长
    // 堆顶和栈顶之间的虚地址空间很大

    //a.命令行参数和环境变量:
    extern char **environ;
    printf("address:\n");
    printf("environ:%p\n", *environ);
    printf("argv:%p\n\n", *argv);

    // b. 栈, 局部变量, 函数调用时寄存器值等都存放在栈中, 函数入参
    int local1 = 1;
    printf("local1:%p argc:%p\n", &local1, &argc);
    int local2 = 2;
    printf("local2:%p\n\n", &local2);

    //c.堆,
    char *p_heap1 = (char*)malloc(100);
    printf("p_heap1:%p\n", p_heap1);
    char *p_heap2 = (char*)malloc(100);
    printf("p_heap2:%p\n\n", p_heap2);
    free(p_heap1);
    free(p_heap2);
    
    //d. .bss 段 (由 exec 初始化为 0)
    int local3;
    printf("local3:%p\n", &local3);
    printf("uninitialize_var1:%p\n\n", &uninitialize_var1);

    //e. data 端 (由 exec 从程序文件中读入)
    printf("initialized_var1:%p\n\n", &initialized_var1);

    //f. text 段(由 exec 从程序文件中读入)
    print_fun_addr(this_is_a_fun);
}

/**
 * @brief 常见的与内存操作有关的问题
 * 1. double free 导致的 dump
 * 2. 操作申请 的内存越界, 导致的 dump(踩内存当时不会出现 dump, 但是以后一定会 dump))
 * 3. 内存泄漏 导致的 dump
 * 4. 使用已释放的内存导致的 dump;
*/
void heap_memory_op_dump_test(void)
{
    //TODO:
    //可用工具: 参考 <Unix 环境高级编程 p167>
    //例如: kasan 等
}


/** 
 * @brief 输入命令然后解析典型代码框架(通常调用栈可能更深, 这里仅作为示例)
 * 调用栈:main -> do_line -> cmd_add
 * 缺点: 当最顶层(栈向下扩展)出现异常时, 要一层一层的返回错误信息
 * 解决措施:使用 非局部 goto 跳过中间的几个栈帧, 直接到达其他栈
 *
 */
#define TOKEN_ADD   9 //'+'
#define TOKEN_SUB   8 //'-'
#define TOKEN_MUL   7 //'*'
#define TOKEN_DIV   6 //'/'
static void do_line(char *ptr);
static void cmd_add(void);
static int get_token(void);
char *tok_ptr;
/**
 * @brief 经典实现
 * 缺点: 当最顶层(栈向下扩展)出现异常时, 要一层一层的返回错误信息
 * 
*/
void classic_cmd_parse_realize(void)
{
    char line[BUFFERSIZE];

    while (fgets(line, MAXLINE, stdin) != NULL) {
        do_line(line);
    }
    exit(0);
}

/**
 * @brief 使用 longjump 优化后的实现
 * 解决措施:使用 非局部 goto 跳过中间的几个栈帧, 直接到达其他栈
 * 
*/
jmp_buf jmpbuffer; // 用来保存恢复栈帧的信息
void jump_test(void)
{
    char line[BUFFERSIZE];

    if (setjmp(jmpbuffer) != 0) {
        err_ret("setjmp failed");
    }

    while (fgets(line, BUFFERSIZE, stdin) != NULL) {
        do_line(line);
    }
    exit(0);
}

/**
 * @brief process one line of input
*/
static void do_line(char *ptr)
{
    int cmd;

    tok_ptr = ptr;
    printf("ptr:%s\n", ptr);
    while ((cmd = get_token()) > 0) {
        switch (cmd) {
            case TOKEN_ADD: {
                cmd_add();
                break;
            }
        }
    }
}

static void cmd_add(void)
{
    int token;

    token = get_token();
    if (token < 0) {
        printf("jump->\n\n");
        longjmp(jmpbuffer, 1);
    }
}

/**
 * @brief 通过 tok_ptr 获取当前字符
*/
static int get_token(void)
{
    return -1;
    // return *tok_ptr++;
}

// longjump 后:自动变量, 寄存器变量的值是不确定的
// volatile 的值不变
// 全局变量/静态局部变量值不变
// static jmp_buf jmpbuffer;
static int globval;
static void display_var(int g, int a, int r, int v, int s);
static void f1(int a, int r, int v, int s);
void jump_test2()
{
    int autoval;
    register int regival;
    volatile int volaval;
    static int statval;

    globval = 1;
    autoval = 2;
    regival = 3;
    volaval = 4;
    statval = 5;
    if (setjmp(jmpbuffer) != 0) {
        printf("after long jump:\n");
        display_var(globval, autoval, regival, volaval, statval);
        exit(0);
    }

    // change variable before longjump
    globval = 101;
    autoval = 102;
    regival = 103;
    volaval = 104;
    statval = 105;
    f1(autoval, regival, volaval, statval);
    exit(0);
}

static void display_var(int g, int a, int r, int v, int s)
{
    printf("globval:%d\n", g);
    printf("autoval:%d\n", a);
    printf("regival:%d\n", r);
    printf("volaval:%d\n", v);
    printf("statval:%d\n", s);
}

static void f1(int a, int r, int v, int s)
{
    printf("f1():\n");
    display_var(globval, a, r, v, s);
    longjmp(jmpbuffer, 1);
}

/**
 * @brief 获取/设置进程的资源限制
*/
void process_resource_limits_test(void)
{
#ifdef RLIMIT_AS
    print_resource_limits(RLIMIT_AS);
#endif
#ifdef RLIMIT_CORE
    print_resource_limits(RLIMIT_CORE);
#endif
#ifdef RLIMIT_CPU
    print_resource_limits(RLIMIT_CPU);
#endif
#ifdef RLIMIT_DATA
    print_resource_limits(RLIMIT_DATA);
#endif
#ifdef RLIMIT_FSIZE
    print_resource_limits(RLIMIT_FSIZE);
#endif
#ifdef RLIMIT_MEMLOCK
    print_resource_limits(RLIMIT_MEMLOCK);
#endif
#ifdef RLIMIT_NICE
    print_resource_limits(RLIMIT_NICE);
#endif
#ifdef RLIMIT_NOFILE
    print_resource_limits(RLIMIT_NOFILE);
#endif
#ifdef RLIMIT_MEMLOCK
    print_resource_limits(RLIMIT_MEMLOCK);
#endif
#ifdef RLIMIT_NOFILE
    print_resource_limits(RLIMIT_NOFILE);
#endif
#ifdef RLIMIT_NPROC
    print_resource_limits(RLIMIT_NPROC);
#endif
#ifdef RLIMIT_NPTS
    print_resource_limits(RLIMIT_NPTS);
#endif
#ifdef RLIMIT_RSS
    print_resource_limits(RLIMIT_RSS);
#endif
#ifdef RLIMIT_SBSIZE
    print_resource_limits(RLIMIT_SBSIZE);
#endif
#ifdef RLIMIT_SIGPENDING
    print_resource_limits(RLIMIT_SIGPENDING);
#endif
#ifdef RLIMIT_STACK
    print_resource_limits(RLIMIT_STACK);
#endif
#ifdef RLIMIT_SWAP
    print_resource_limits(RLIMIT_SWAP);
#endif
#ifdef RLIMIT_VMEM
    print_resource_limits(RLIMIT_VMEM);
#endif
}


