/*
 *	POSIX Standard: 7.1-2 General Terminal Interface	<termios.h>
 */
#include <termios.h>
// POSIX 标准前的 SystemV 版本的遗留, 为了与之前版本区别, POSIX.1 添加了一个 s
#include <termio.h>

#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#if defined(SOLARIS)
#include <stropts.h>
#endif

#include "ch18.h"

// shell: 规范模式
// vi:非规范模式

/***************************************************************
 * Macro
 ***************************************************************/
#ifdef LINUX
#define OPTSTR  "+d:einv"
#else
#define OPTSTR  "d:einv"
#endif

struct devdir {
    struct devdir *d_next;
    char *d_name;
};

// #define ECHO	0000010
// set
#define SET_BIT(val, bit)   (val |= (1 << bit))  // 设置第 bit 位为 1
#define CLEAR_BIT(val, bit) (val &= ~(1 << bit)) // 设置第 bit 位为 0

/***************************************************************
 * VARIABLES
 ***************************************************************/
// 非规范模式
static struct termios save_termios; // save previous terminal settings
static int ttysavefd = -1;
static enum {
    RESET, 
    RAW, 
    CBREAK
} ttystate = RESET;

static volatile sig_atomic_t sigcaught;
/***************************************************************
 * Public Function
 ***************************************************************/
// 终端属性控制
extern int tcgetattr(int fd, struct termios *termptr);
extern int tcsetattr(int fd, int opt, const struct termios *termptr);

//波特率控制
extern speed_t cfgetispeed(const struct termios *termptr);
extern speed_t cfgetospeed(const struct termios *termptr);
extern int cfsetispeed(struct termios *termptr, speed_t speed);
extern int cfsetospeed(struct termios *termptr, speed_t speed);

// 行控制函数
extern int tcdrain(int fd);
extern int fcflow(int fd, int action);
extern int fcflush(int fd, int queue);
extern int tcsendbreak(int fd, int duration);

extern char *ctermid(char *ptr); // 可用来确定控制终端的名字
extern int isatty(int fd); // 是否是终端设备
extern char *ttyname(int fd); // 返回在该文件描述符打开的终端设备路径名

extern int posix_openpt(int oflag); // 打开一个伪终端主设备
extern int grantpt(int fd); // 更改主设备 fd 对应的从设备的权限
extern int unlockpt(int fd); // 允许打开主设备 fd 对应的从设备
extern char *ptsname(int fd); // 返回从设备路径名
/***************************************************************
 * Private Function Declaration
 ***************************************************************/
static char *ctermid_m(char *str);
static int isatty_m(int fd);
static char *ttyname_m(int fd);
static char *getpass_m(const char *prompt);

static void add(char *dirname);
static void cleanup(void);
static char *searchdir(char *dirname, struct stat *fdstatp);
static void sig_catch(int signo);
static void sig_winchange_hander(int signo);
static void print_window_size(int fd);
static void sig_term_handler(int signo);

static void set_noecho(int); // at the end of this file
void do_driver(char *); // int the file driver.c

static void sig_term_handler(int signo);
static void set_noecho(int fd);

void tty_atexit(void);
/***************************************************************
 * Public Variables
 ***************************************************************/
// struct termios {
//     tcflag_t c_iflag; // 输入标志, 通过终端设备驱动程序控制字符输入
//     tcflag_t c_oflag; // 输出标志, 控制驱动程序输出
//     tcflag_t c_cflag; // 控制标志, 影响 RS-232 串行线
//     tcflag_t c_lflag; // 本地标志, 影响驱动程序和用户之间的接口, 例如:回显打开, 猜出字符等
//     cc_t c_cc[NCCS]; // 包含了所有可以更改的特殊字符
// };
/***************************************************************
 * Private Variables
 ***************************************************************/
/**
 * @brief 返回特殊字符字符串
*/
static const char* _get_special_char_str(int idx)
{
    switch (idx) {
        CASE_RET_STR2(VINTR);
        CASE_RET_STR2(VQUIT);
        CASE_RET_STR2(VERASE);
        CASE_RET_STR2(VKILL);
        CASE_RET_STR2(VEOF);
        CASE_RET_STR2(VTIME);
        CASE_RET_STR2(VMIN);
        CASE_RET_STR2(VSWTC);
        CASE_RET_STR2(VSTART);
        CASE_RET_STR2(VSTOP);
        CASE_RET_STR2(VSUSP);
        CASE_RET_STR2(VEOL);
        CASE_RET_STR2(VREPRINT);
        CASE_RET_STR2(VDISCARD);
        CASE_RET_STR2(VWERASE);
        CASE_RET_STR2(VLNEXT);
        CASE_RET_STR2(VEOL2);
        // default:
        //     return "unknown idx";
        default:
            return "undef";
    }
}

/**
 * @brief 打印终端有关的所有信息
 * 
*/
void print_term_info(int fd)
{
    struct termios term;

    if (isatty(fd) == false) {
        fprintf(STDERR_FILENO, "%d is not a terminal", fd);
        return;
    }

    if (tcgetattr(fd, &term) < 0) {
        perror("get terminal info failed");
        return;
    }
    
    // TODO:
    // print other information
    
    for (int i = 0; i < NCCS; ++i) {
        printf("c_cc[%d] %s=%d\n", i, _get_special_char_str(i),term.c_cc[i]);
    }

    printf("\n\n");

    return;
}


// 保存终端名字
static char ctermid_name[L_ctermid];
static struct devdir *head = NULL;
static struct devdir *tail = NULL;
static char pathname[_POSIX_PATH_MAX + 14];
static volatile sig_atomic_t sigcaught;
/**
 * @fun: 禁用中断字符 ^C, ^?
 *       文件结束符号 ^D 改为 ^B
 * 运行这个可执行程序后的效果:1. terminal 无法退出:^D 失效; 2. ^C 失效
*/
void change_term_stat(void)
{
    struct termios term;
    long vdisable; // 特殊字符处理功能是否可以被关闭

    if (isatty(STDIN_FILENO) == 0)
        err_quit("standard input is not a terminal device");
    
    // _PC_VDISABLE:returns nonzero if special character processing can be disabled
    if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) < 0)
        err_quit("fpathcof erroror _POSIX_VDISABLE not in effect");
    printf("vdisable:%d\n\n", vdisable);

    print_term_info(STDIN_FILENO);

    if (tcgetattr(STDIN_FILENO, &term) < 0)
        err_sys("tcgetattr error");

    printf("previous:\n");
    printf("[%d]:%X\n", VINTR, term.c_cc[VINTR]); // 3
    printf("[%d]:%X\n", VEOF, term.c_cc[VEOF]); // 4
    term.c_cc[VINTR] = vdisable; // 0:disable INTR character
    term.c_cc[VEOF] = CTRL_B; // EOF set to ^B
    printf("after:\n");
    printf("[%d]:%X\n", VINTR, term.c_cc[VINTR]); // 0
    printf("[%d]:%X\n", VEOF, term.c_cc[VEOF]); // 2
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0)
        err_sys("tcsetattr error");
    
    exit(0);
}

 
/**
 * @brief 设置当前终端的字符长度
 * O_TTY_INIT
 * $ stty 命令也可以检查和更改终端选项, 前面有个连字符表示该选项禁用
*/
void term_state_flag(void)
{
    struct termios term;

    if (tcgetattr(STDIN_FILENO, &term) < 0)
        err_sys("tcgetattr error");
    
    // #define CSIZE	0000060
    //CSIZE bit[5:6]
    switch (term.c_cflag & CSIZE) {
        case CS5:
            printf("5 bits/byte\n");
        break;
        case CS6:
            printf("6 bits/byte\n");
        break;
        case CS7:
            printf("7 bits/byte\n");
        break;
        case CS8:
            printf("8 bits/byte\n");
        break;
        default:
            printf("unknown bits/byte\n");
        break;
    }

    term.c_lflag &= ~CSIZE; // clear
    term.c_cflag |= CS8; // set 8bits/byte
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0)
        err_sys("tcsetattr error");
    
    return;
}

/**
 * @brief 设置规范模式
 * ICANON 标志设置后, 使得下列字符起作用
 * EOF, EOL, EOL2, ERASE, KILL, REPRINT,STATUS and WERASE, 输入字符被装配成行
 * 如果不以规范模式工作, 则读请求直接从输入队列获取字符, 在至少接收到 MIN 个字符或者两个字节之间的超时值 TIME 到期时
 * read 才返回
 * 
*/
void term_ICANON_test(void)
{
    struct termios term;

    if (isatty(STDIN_FILENO) == false) {
        perror("not an atty");
        return;
    }
    
    if (tcgetattr(STDIN_FILENO, &term) < 0) {
        perror("get attr");
        return;
    }
    print_term_info(STDIN_FILENO);

    printf("before ICANON:%d\n", term.c_lflag & ICANON);

    term.c_lflag &= ~ICANON;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
        perror("set failed");
        return;
    }

    printf("after ICANON:%d\n", term.c_lflag & ICANON);

    return;
}

/**
 * @brief 波特率
*/
void term_speed_test(void)
{
    struct termios term;

    if (isatty(STDIN_FILENO) == false) {
        perror("not an atty");
        return;
    }
    
    if (tcgetattr(STDIN_FILENO, &term) < 0) {
        perror("get attr");
        return;
    }

    printf("before:%d %d\n", cfgetispeed(&term), cfgetospeed(&term));

    cfsetispeed(&term, B921600);
    cfsetospeed(&term, B921600);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
        perror("tcsetattr");
        return;
    }

    printf("after:%d %d\n", cfgetispeed(&term), cfgetospeed(&term));

    return;
}

/**
 * @brief 
*/
void term_io_test1(void)
{
    // tcdrain();
    // tcflow();
    // tcflush();
    // tcsendbreak();
}

/**
 * @brief 获取终端设备名字
 * Q:电脑上打开很多个控制终端, 所有的终端名字都一样..
 * A:ttyname
*/
void term_get_name(void)
{
    printf("terminal name:%s\n", ctermid(NULL));  // /dev/tty
    ctermid(ctermid_name);
    printf("ctermid_name:%s\n\n", ctermid_name);

    printf("ttyname:%s\n", ttyname(STDIN_FILENO)); // /dev/pts/21
    // printf("ttyname:%s\n", ttyname(STDOUT_FILENO));
}

/**
 * @brief 
 * 
 * $ ./test  < /etc/passwd 2> /dev/null
 * 标准输入 & 错误重定向后, 不再是终端设备
*/
void tty_test(void)
{
    printf("terminal name:%s\n", ttyname(STDIN_FILENO));
    // printf("fd 0:%s\n", isatty(0) ? "tty" : "not a tty");
    // printf("fd 1:%s\n", isatty(1) ? "tty" : "not a tty");
    // printf("fd 2:%s\n", isatty(2) ? "tty" : "not a tty");
    printf("fd 0:%s\n", isatty_m(0) ? "tty" : "not a tty");
    printf("fd 1:%s\n", isatty_m(1) ? "tty" : "not a tty");
    printf("fd 2:%s\n", isatty_m(2) ? "tty" : "not a tty");

    // printf("terminal name:%s\n", ctermid(NULL));
    // printf("_POSIX_PATH_MAX:%d\n", _POSIX_PATH_MAX);
}

/**
 * @fun: 获取终端的名字
 * 可能的实现
*/
static char *ctermid_m(char *str)
{
    if (str == NULL);
        str = ctermid_name;
    return (stpcpy(str, "/dev/tty"));
}

/**
 * @fun 判断一个描述符是否是终端
*/
static int isatty_m(int fd)
{
    struct termios ts;

    return (tcgetattr(fd, &ts) != -1);
}

/**
 * @brief 获取 ttyname,搜索所有的设备表项 /dev 文件夹, 寻找匹配项
 *
 * @param[in] fd 要查找的终端描述符
 * @return The function ttyname() returns a pointer to a pathname on success.  
 * On error, NULL is returned, and errno is set appropriately.  
 * The function ttyname_r() returns 0 on success,  and  an  error number upon error.
*/
static char *ttyname_m(int fd)
{
    struct stat fdstat;
    struct devdir *ddp; // directory pointer
    char *rval;

    if (isatty(fd) == false)
        return (NULL);

    if (fstat(fd, &fdstat) < 0) // get fd stat
        return (NULL);
    if (S_ISCHR(fdstat.st_mode) == 0)
        return (NULL);

    // 递归遍历文件夹
    rval = searchdir("/dev", &fdstat);
    if (rval == NULL) {
        for (ddp = head; ddp != NULL; ddp = ddp->d_next) {
            // 遍历 /dev hulu, 寻找具有相同 i 节点和设备号的表项
            if ((rval = searchdir(ddp->d_name, &fdstat)) != NULL)
                break;
        }
    }

    cleanup();

    return (rval);
}

/**
 * @brief 将文件夹路径添加到链表中, 用于后续遍历查找指定 主设备号/次设备号的终端
 *
 * @param[in] dirname 文件夹路径
 * @return None
*/
static void add(char *dirname)
{
    struct devdir *ddp;
    int len;

    len = strlen(dirname);
    
    //TODO: 
    // /dev 目录文件结构
    // /proc 目录文件结构


    // skip ., .., /dev/fd
    // 跳过这些文件夹, 都是符号链接, 指向同一个文件夹
    // dr-x------ 2 qz qz  0 6月   8 08:46 ./
    // dr-xr-xr-x 9 qz qz  0 6月   8 08:46 ../
    // lrwx------ 1 qz qz 64 6月   8 08:46 0 -> /dev/pts/20
    // lrwx------ 1 qz qz 64 6月   8 08:46 1 -> /dev/pts/20
    // lrwx------ 1 qz qz 64 6月   8 08:46 2 -> /dev/pts/20
    // lrwx------ 1 qz qz 64 6月   9 13:16 255 -> /dev/pts/20
    if ((dirname[len-1] == '.') && 
        (dirname[len-2] == '/' || (dirname[len-2] == '.' && dirname[len-3] == '/')))
            return;
    
    if (strcmp(dirname, "/dev/fd") == 0)
        return;

    if ((ddp = malloc(sizeof(struct devdir))) == NULL)
        return;
    if ((ddp->d_name = strdup(dirname)) == NULL) { // strdup() automaically allocate memory with malloc()
        free(ddp);
        return;
    }

    // linklist operation:插入到尾部
    printf("add a linklist noe:%s\n", dirname);
    ddp->d_next = NULL;
    if (tail == NULL) {
        head = ddp;
        tail = ddp;
    } else {
        tail->d_next = ddp;
        tail = ddp;
    }
}

/**
 * @fun 清空链表
*/
static void cleanup(void)
{
    struct devdir *ddp, *next_ddp;

    ddp = head;
    while (ddp != NULL) {
        next_ddp = ddp->d_next;
        free(ddp->d_name); // allocate by strdup()
        free(ddp);
        ddp = next_ddp;
    }
    head = NULL;
    tail = NULL;
    printf("\n\ncleanup\n\n");
}

/**
 * @brief 在指定目录下查找与该文件设备号匹配的文件路径
 * 
 * @param[in] dirname directory name
 * @param[in] fdstatp stat 要查找的文件信息
 * 
 * @ret 成功,返回文件信息, 失败, 返回 NULL
*/
static char *searchdir(char *dirname, struct stat *fdstatp)
{
    struct stat devstat;
    DIR *dp;
    int devlen;
    struct dirent *dirp;

    strcpy(pathname, dirname); // include '\0'
    if ((dp = opendir(dirname)) == NULL)
        return (NULL);
    strcat(pathname, "/"); // "/dev" -> "/dev/"
    devlen = strlen(pathname);

    // 遍历目录
    while ((dirp = readdir(dp)) != NULL) {
        strncpy(pathname + devlen, dirp->d_name, _POSIX_PATH_MAX - devlen);
        printf("pathname:%s\n", pathname);
        // skip aliases, 跳过这些符号链接文件
        // lrwxrwxrwx  1 root root            15 6月   7 22:32 stderr -> /proc/self/fd/2
        // lrwxrwxrwx  1 root root            15 6月   7 22:32 stdin -> /proc/self/fd/0
        // lrwxrwxrwx  1 root root            15 6月   7 22:32 stdout -> /proc/self/fd/1
        if (strcmp(pathname, "/dev/stdin") == 0 || // 跳过 stdin, stdou, sterr
            strcmp(pathname, "/dev/stdout") == 0 ||
            strcmp(pathname, "/dev/stderr") == 0)
                continue;

        if (stat(pathname, &devstat) < 0) // 获取当前 path 的 stat
            continue;

        if (S_ISDIR(devstat.st_mode)) { // 文件夹添加到链表中,用于后续继续遍历查找
            add(pathname);
            continue;
        }

        // i 节点编号 和 主设备号匹配(次设备号也可以用于判断)
        if (devstat.st_ino == fdstatp->st_ino && /* 文件系统中每一个目录项都有唯一的 i 节点编号:File serial number.	*/
            devstat.st_dev == fdstatp->st_dev && /* 每个文件系统都有惟一的 主设备号 Device.  */
            devstat.st_rdev == fdstatp->st_rdev ) { // 也可以考察次设备号
            closedir(dp);
            return (pathname);
        }
    }
    closedir(dp);

    return (NULL);
}

/**
 * @brief 拷贝字符串, 并且自动分配内存(使用 malloc 分配), 分配的内存需要我们手动释放
 * 不使用的时候需要手动 free(), 否则会造成内存泄漏
*/
void strdup_test(void)
{
    char path[10] = "abc";
    char *ptr = strdup(path);

    printf("ptr:%s path:%s\n", ptr, path);
    ptr[0] = 'e';
    printf("ptr:%s path:%s\n", ptr, path);
    
    free(ptr);

    return;
}

/**
 * @brief ttyname_m test
*/
void ttyname_test(void)
{
    char *name;
    
    // 0, 1, 2
    for (int fd_ix = 0; fd_ix <= 2; ++fd_ix) {
        if (isatty(fd_ix)) {
            // name = ttyname(fd_ix);
            name = ttyname_m(fd_ix);
            if (name == NULL)
                name = "undefined";
        } else {
            name = "not a tty";
        }
        printf("fd %d name:%s\n",fd_ix, name);
    }
    exit(0);
}

/**
 * @brief getpass_m test
*/
void getpass_test(void)
{
    char *ptr;

    // if ((ptr = getpass("Enter a passwd:")) == NULL) {
    if ((ptr = getpass_m("Enter a passwd:\n")) == NULL) {
        err_sys("get passwd failed");;
    }

    //TDDO:
    // now usse passwork(probably encrypt it)
    // 最好不要保存明文密码,要进行加密操作,防止被他人窃取

    printf("passwd:%s\n", ptr);
    while (*ptr != 0) {
        *ptr++ = 0;
    }

    exit(0);
}

// The getpass() function opens /dev/tty (the controlling ter‐
// minal of the process), outputs the string prompt, turns off
// echoing, reads one line (the "password"), restores the ter‐
// minal state and closes /dev/tty again.

/**
 * @brief 规范模式(按行返回,对特殊字符进行处理):getpass() 可能的实现
 *      1.关闭回显,
 *      2.忽略 SIGINT, SIGSTOP, 所有 UNIX 版本的实现都没有忽略 SIGQUIT
 *      3.设置为无缓冲的
 *      4.读完以后恢复信号
*/
static char *getpass_m(const char *prompt)
{
#define MAX_PASS_LEN        8   // 密码最大长度
    static char buf[MAX_PASS_LEN + 1]; // '\0' at end
    char *ptr;
    sigset_t sig, old_sig;
    struct termios ts, old_ts;
    FILE *fp;
    int c;
    char *term_name = ctermid(NULL); // dev/tty

    printf("open terminal:%s\n", term_name);
    if ((fp = fopen(term_name, "r+")) == NULL) // 读写方式打开一个终端
        return (NULL);
    setbuf(fp, NULL); // 设置无缓冲

    // 屏蔽:^C ^S 会导致程序异常终止, 但是回显仍然是打开状态, 导致程序异常;
    // getpass 结束后, 恢复信号
    // 没有一个版本阻塞:^Q, 所以, ^Q 会导致程序异常退出, 且处于关闭回显状态
    sigemptyset(&sig);
    sigaddset(&sig, SIGINT);
    sigaddset(&sig, SIGTSTP);
    sigprocmask(SIG_BLOCK, &sig, &old_sig); // block SIGINT, SIGTSTP

    tcgetattr(fileno(fp), &ts);
    old_ts = ts;
    ts.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    tcsetattr(fileno(fp), TCSAFLUSH, &ts); // 关闭回显, 否则输入
    fputs(prompt, fp); // 打印提示内容

    ptr = buf;
    while ((c = getc(fp)) != EOF && c != '\n') { // block
        // printf("c:%c\n", c);
        if (ptr < &buf[MAX_PASS_LEN]) // 规范输入模式:锁请求的字节数已到时;读到 NL 时; 读返回
            *ptr++ = c;
        // shell:<规范模式>,行打印或者读到指定的字节数返回
        else {
            // 这里不可以使用 perror("break");
            // perror(""); 只有在系统调用出错的时候才会打印出错误信息, 否则会打印 Success
            fprintf(stderr, "\nEnter Too Long!!!\n");
            break;
        }
    }
    *ptr = 0;
    putc('\n', fp); // we echo a newline

    tcsetattr(fileno(fp), TCSAFLUSH, &old_ts); // restore TTY state
    sigprocmask(SIG_SETMASK, &old_sig, NULL); // restore signal mask
    fclose(fp);

    printf("exit\n");
    return (buf);
}

/**
 * @brief 终端 cbreak 模式:
 * 1.非规范模式, 调用者应该捕获信号,否则可能会导致程序由于信号推出后(在信号处理函数中恢复终端设置), 终端一直处于 cbreak 模式
 * 2.关闭回显
 * 3.每次输入一个字节(VMIN设置为1即可)
 * 
 * @note cbreak 不对输入特殊字符进行处理,,因此没有对 ^D, 文件结束已经退格符进行特殊处理, 但是仍对信号进行处理
 * @return 
*/
int tty_cbreak(int fd)
{
    int err;
    struct termios buf;

    if (ttystate != RESET) {
        errno = EINVAL;
        perror("invalid state");
        return (-1);
    }

    if (tcgetattr(fd, &buf) < 0) {
        perror("get attr");
        return (-1);
    }
    save_termios = buf; // save previous terminal setting

    buf.c_lflag &= ~(ECHO | ICANON); // Echo off, canonical mode off
    // case B:1 byte at a time, no timer
    buf.c_cc[VMIN] = 1; // 最小字节数
    buf.c_cc[VTIME] = 0; // 没有超时时间, 如果接收到指定字节数就无限期阻塞
    if (tcsetattr(fd, TCSAFLUSH, &buf) < 0) {
        return (-1);
    }

    // 设置完后确认一下是否设置成功
    if (tcgetattr(fd, &buf) < 0) {
        err = errno;
        tcsetattr(fd, TCSAFLUSH, &save_termios); // restore terminal setting
        errno = err;
        return (-1);
    }

    // only some of changes were make, resore the original settings
    if ((buf.c_lflag & (ECHO | ICANON)) || 
            buf.c_cc[VMIN] != 1 ||
            buf.c_cc[VTIME] != 0) {
        perror(" only some of changes were make");
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = EINVAL;
        return (-1);
    }

    ttystate = CBREAK; // save global terminal flag
    ttysavefd = fd;

    return (0);
}

/**
 * @brief 切换到 tty_raw 模式
 * 1. 非规范模式
 * 2. 关闭回显
 * 3. 禁止 CR 到 NL 的映射(ICRNL), 输入奇偶校验(INPCK), 剥离 8 bytes 的第 8 位(ISTRIP)以及输出流控制(IXON)
 * 4. 进制所有输出处理(OPOSE)
 * 5. 每次输入一个字符(MIN=1, TIME=0)
 * 
 * @note rawmode 关闭了输出处理 OPOST, 所以输出没有得到回车符, 输入 Enter 只得到了换行符, 没有得到回车符.
 * @return returns 0 if success; otherwise a negative number is returned and errno is set to
 *         indicate the error
*/
int tty_raw(int fd)
{
    int err;
    struct termios buf;

    if (ttystate != RESET) {
        errno = EINVAL;
        perror("invalid state");
        return (-1);
    }

    if (tcgetattr(fd, &buf) < 0) {
        fprintf(stderr, "getattr failed\n");
        return (-1);
    }

    save_termios = buf; // structure copy, save initial state for restore

    // Echo off, canonical mode off, extended input processing off, signal chars off
    // 关闭回显, 关闭规范模式, 关闭扩充输入字符处理, 对特殊字符处理(输入特殊字符不会产生信号)
    buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // No SIGINT on BREAK, CR-to-NL off, input parity check off, don't strip 8th bit on input, output flow control off
    // BREAK 字符行为有关, 关闭 CR->NL,关闭奇偶校验, 处理 8 位,关闭START(^Q)/STOP(^S)控制
    buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // clear size bits, parity checking off
    buf.c_cflag &= ~(CSIZE | PARENB);
    // set 8 bits per char
    buf.c_cflag |= CS8;
    // output processing off,
    //关闭输出处理功能. 若设置, 则进行 c_oflag 实现的定义输出处理 
    buf.c_oflag &= ~(OPOST);
    buf.c_cc[VMIN] = 1; // 1bytes at a timer, no timer
    buf.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
        return (-1);
    
    // 确认配置是否生效
    if (tcgetattr(fd, &buf) < 0) {
        err = errno;
        perror("tcgetattr failed");
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno =err;
        return (-1);
    }

    if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) ||
            (buf.c_iflag & (BRKINT | ICRNL | INPCK | ISTRIP | IXON)) ||
            (buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8 ||
            (buf.c_oflag & (OPOST)) || /* 关闭输出处理 */
            (buf.c_cc[VMIN] != 1 || buf.c_cc[VTIME] != 0)) {
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = EINVAL;
        return (-1);
    }

    ttystate = RAW;
    ttysavefd = fd;

    return (0);
}

/**
 * @brief reset terminal settings
*/
int tty_reset(int fd)
{
    if (ttystate == RESET)
        return (0);

    if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0) {
        fprintf(stderr, "reset failed");
        return (-1);
    }

    ttystate = RESET;

    fprintf(stderr, "\n\nreset tty fd:%d\n\n", fd);

    return (0);
}

/**
 * @brief 设置退出程序(退出的时候会调用,一般用与恢复终端原始设置)
*/
void tty_atexit(void)
{
    if (ttysavefd >= 0)
        tty_reset(ttysavefd);
}

/**
 * @brief 非规范模式, 模式切换方法:
 * tty_cbreak() -> tty_reset() -> tty_raw()
 * tty_raw() -> tty_reset() -> tty_cbreak()
*/
void abnormal_terminal_test(void)
{
    int i; 
    char c;

    // raw mode
    if (signal(SIGINT, sig_catch) == SIG_ERR)
        err_sys("register signal handler failed");
    if (signal(SIGQUIT, sig_catch) == SIG_ERR)
        err_sys("register signal handler failed");
    if (signal(SIGTERM, sig_catch) == SIG_ERR)
        err_sys("register signal handler failed");    

    if (tty_raw(STDIN_FILENO) < 0)
        err_ret("tty_raw error");
    fprintf(stderr, "\n\nenter raw mode character, Terminate with \"DELETE\"\n\n");
    while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
        // TODO:
        // 这里为什么是 177? 输入 delete 根本无法退出
        // if ((c &= 0xFF) == 0177) // 0177 = ASCII Delete
        if ((c &= 0xFF) == 0176) // 0177 = ASCII Delete
        // if ((c &= 0xFF) == 0x04) // ^C = ASCII Delete
            break;
        printf("%o(%c)\n", c, c);
    }

    // change to cbreak mode
    if (tty_reset(STDIN_FILENO) < 0)
        err_ret("tty_reset failed");
    if (i <= 0)
        err_ret("read error");
    if (tty_cbreak(STDIN_FILENO) < 0)
        err_sys("tty_cbreak failed");
    fprintf(stderr, "\n\nEnter cbreak mode, exit with ^C(SIGINT)\n\n");
    while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
        c &= 0xFF;
        printf("%o(%c)\n", c, c);
    }
    if (tty_reset(STDIN_FILENO) < 0)
        err_ret("tty_reset failed");
    if (i <= 0)
        err_ret("read error");

    exit(0);
}

static void sig_catch(int signo)
{
    fprintf(stderr, "signal caught:%d\n", signo);
    tty_reset(STDIN_FILENO);
    exit(0);
}

/**
 * @brief set window size
*/
void winsize_test(void)
{
    if (isatty(STDIN_FILENO) == 0)
        exit(1);
    if (signal(SIGWINCH, sig_winchange_hander) == SIG_ERR)
        err_ret("register signal failed");
    print_window_size(STDIN_FILENO);
    for (;;)
        pause();
}

static void sig_winchange_hander(int signo)
{
    printf("signal  window size change\n");
    print_window_size(STDIN_FILENO);
}

/**
 * @brief 参考驱动实现, 上层通过 ioctl 向底层发控制命令
 * TIOCGWINSZ
 * TODO:
 * 做 Linux 驱动时, 如果注册自定义的 cmd ???
*/
static void print_window_size(int fd)
{
    struct winsize size;

    if (ioctl(fd, TIOCGWINSZ, (char*)&size) < 0)
        err_sys("TIOCGWINSZ error");
    printf("%d rows, %d colums pixles x:%d y:%d\n", 
            size.ws_row, size.ws_col, size.ws_xpixel, size.ws_ypixel);
}

/******************************* psedo terminal *********************************************/
/**
 * 伪终端:
 * 对于应用程序而言, 它看上去向一个终端, 但它实际上并不是一个终端
 * 典型使用场景:
 * 一个进程打开伪终端主设备, 然后调用 fork()
 * 子进程建立一个新的会话(自动成为进程组组长/会话首进程), 打开一个相应的伪终端从设备, 将其文件描述符复制到 stdin/stdout/stderr 
 * 然乎调用 exec(), 伪终端从设备成为子进程的控制终端(TODO:所以呢, 有什么用呢???)
 * 
 * 对于伪终端从设备进程来说, 其 stdin/stdout/stderr 都是伪终端设备, 用户进程可以调用所有 ch18 中的终端有关的函数
 * 
 * 任何写到伪终端主设备的都会作为从设备的输入, 反之亦然(写到终端从设备都会作为主设备的输入)
 * 着看起来就像一个双向管道(额外增加了终端行规程部分, 普通管道没有的能力)
 * 
*/
/**
 * @brief  打开一个伪终端并且返回从设备名称
 * 由于 posix_openpt()返回的名称是保存在静态内存里的,所以需要马上拷贝出去, 否则会被覆盖
 *
 * @param[out] pts_name 保存从设备名称
 * @param[in] pts_namesz
 * @ret 返回主设备 fd
*/
int ptym_open(char *pts_name, int pts_namesz)
{
    char *ptr;
    int fdm, err;

    if ((fdm = posix_openpt(O_RDWR)) < 0)
        return (-1);
    if (grantpt(fdm) < 0) // grant access to slave
        goto errout;
    if (unlockpt(fdm) < 0) // clear slave's lock flag
        goto errout;
    if ((ptr = ptsname(fdm)) == NULL)
        goto errout;
    
    // copy name of slave
    strncpy(pts_name, ptr, pts_namesz);
    pts_name[pts_namesz - 1] = '\0';
    return (fdm);
errout:
    err = errno;
    close(fdm);
    errno = err;
    return (-1);
}

/**
 * @brief 打开从设备
 *      从设备必须在 unlockpt() 之后才可以打开
 * @param[in] pts_name
*/
int ptys_open(char *pts_name)
{
    int fds;
#if defined(SOLARIS)
    int err, setup;
#endif

    if ((fds = open(pts_name, O_RDWR)) < 0)
        return (-1);
#if defined(SOLARIS)
    // check if stream is already setup by auto push facility
    if ((setup = ioctl(fds, I_FIND, "ldterm")) < 0)
        goto errout;

    // SOLARIS 需要手动添加一些 stream 模块
    if (setup == 0) {
        if (ioctl(fds, I_PUSH, "ptem") < 0)
            goto errout;
        if (ioctl(fds, I_PUSH, "ldterm") < 0)
            goto errout;
        if (ioctl(fds, I_PUSH, "ttcompat") < 0)
            goto errout;    
    }

errout:
    err = errno;
    close(fds);
    errno = err;
    return (-1);
#endif

    return (0);
}

/**
 * @fun fork() 调用打开主设备和从设备, 创建作为会话首进程的子进程 并使其具有控制终端
 * @param[out] ptrfdm 主设备描述符
 * @param[out] slave_name
 * @param[in] slave_namesz
 * @param[in] slave_termios 指定一些终端的属性
 * @param[in] slave_winsize 指定终端大小
 * @ret pid
*/
pid_t pty_fork(int *ptrfdm, char *slave_name, int slave_namesz,
                const struct termios *slave_termios, const struct termios *slave_winsize)
{
    int fdm, fds;
    pid_t pid;
    char pts_name[20];

    if ((fdm = ptym_open(pts_name, sizeof(pts_name))) < 0)
        err_sys("Can't open master pty:%s err:%d\n", pts_name, fdm);
    if (slave_name != NULL) {
        strncpy(slave_name, pts_name, slave_namesz);
        slave_name[slave_namesz - 1] = '\0';
    }

    if ((pid = fork()) < 0) {
        return (-1);
    } else if (pid == 0) { // child
        if (setsid() < 0) // 创建子进程作为会话首进程
            err_sys("setsid error");

        // 控制终端, 打开从设备
        if ((fds = ptys_open(pts_name)) < 0)
            err_sys("can't open slave");
        close(fdm); // all donw with master in child
#if defined(BSD)
        // TIOCSCTTY is the BSD way to acquire a controlling terminal
        if (ioctl(fds, TIOCSCTTY, (char*)0) < 0)
            err_sys("TIOCSCTTY error");
#endif
        
        // set slave's termios and window size
        if (slave_termios != NULL) {
            if (tcsetattr(fds, TCSANOW, &slave_termios) < 0)
                err_sys("tcssetattr error on slave pty");
        }
        if (slave_winsize != NULL) {
            if (ioctl(fds, TIOCSWINSZ, slave_winsize) < 0)
                err_sys("TIOCSWINSZ error on slave pty");
        }

        // slave becomes stdin/stdout/stderr of child
        if (dup2(fds, STDIN_FILENO) != STDIN_FILENO)
            err_sys("dup2 error to stdin");
        if (dup2(fds, STDOUT_FILENO) != STDOUT_FILENO)
            err_sys("dup2 error to stdin");
        if (dup2(fds, STDERR_FILENO) != STDERR_FILENO)
            err_sys("dup2 error to stdin");
        if (fds != STDIN_FILENO && fds != STDOUT_FILENO && fds != STDERR_FILENO) {
            printf("close fds:%d\n", fds);
            close(fds);
        }
        return (0); // child return 0 just like fork() return
    } else { // parent
        *ptrfdm = fdm;
        return (pid); // parent returns pid of child
    }
}

/**
 * @fun 使用 pty 程序可以用 pty prog arg1 arg2 代替 prog arg1 arg2
 * 使用 pty 来执行程序
*/
int pty_test(int argc, char *argv[])
{
    int fdm;
    int c, ignoreeof, interactive, noecho, verbose;
    pid_t pid;
    char *driver;
    char slave_name[20];
    struct termios orig_termios;
    struct winsize size;

    interactive = isatty(STDIN_FILENO);
    ignoreeof = 0; // 一些选项的初始化
    noecho = 0;
    verbose = 0;
    driver = NULL;
    
    opterr = 0;
    while ((c = getopt(argc, argv, OPTSTR)) != EOF) {
        switch (c) {
            case 'd': // driver for stdin/stdout
                driver = optarg;
                break;
            case 'e': // no echo for slave pty's line discipline
                noecho = 1;
                break;
            case 'i': // ignore EOF on standard input
                ignoreeof = 1;
                break;
            case 'n': // not interactive
                interactive = 0;
                break;
            case 'v': // verbose
                verbose = 1;
                break;
            case '?':
                err_quit("unrecognized option:-%c", optopt);
        }
    }
    if (optind >= argc)
        err_quit("usage:pty [-d driver -eint] program [arg...]");
    
    if (interactive) { // fetch current termios and window size
        if (tcgetattr(STDIN_FILENO, &orig_termios) < 0)
            err_quit("tcgetattr() error on stdin");
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char*)&size) < 0)
            err_quit("get window size failed");
        pid = pty_fork(&fdm, slave_name, sizeof(slave_name), &orig_termios, &size); // 带参数
    } else {
        pid = pty_fork(&fdm, slave_name, sizeof(slave_name), NULL, NULL);
    }

    if (pid < 0) {
        err_sys("fork error");
    } else if (pid == 0) { // child
        if (noecho)
            set_noecho(STDIN_FILENO);
        if (execvp(argv[optind], &argv[optind]) < 0)
            err_sys("can't execute:%s", argv[optind]);
    }
    // else { // parent
    // }
    
    // 对不同的参数进行设置
    fprintf(stderr, "parent running\n");
    if (verbose) {
        fprintf(stderr, "slave name = %s\n", slave_name);
        if (driver != NULL)
            fprintf(stderr, "driver = %s\n", driver);
    }

    if (interactive && driver == NULL) {
        if (tty_raw(STDIN_FILENO) < 0) // 非规范模式
            err_sys("tty_raw error");
        if (atexit(tty_atexit) < 0)
            err_sys("atexit error");
    }
    if (driver)
        do_driver(driver); // changes our stdin/stdout
    loop2(fdm, ignoreeof); // copies stdin->ptym, ptym->stdout

    exit(0);
}

/**
 * @fun 关闭从设备回显
 * @param[in] fd
 * @ret
*/
static void set_noecho(int fd)
{
    struct termios stermios;
    
    if (tcgetattr(fd, &stermios) < 0)
        err_sys("tcgetattr error");
    stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    stermios.c_oflag &= ~(ONLCR); // turn off NL to CR/NL mapping on output
    if (tcsetattr(fd, TCSANOW, &stermios) < 0)
        err_sys("tcsetattr error");
}

void do_driver(char *driver)
{
    fprintf(stderr, "driver:%s\n", driver);
}

/**
 * @fun 从标准输入接收所有内容复制到 pty 主设备,并将 pty 主设备接收到的所有数据复制到标准输出
 * 可以使用 select()/poll()实现, 这里使用两个进程实现
*/
void loop2(int ptym, int ignoreeof)
{
    pid_t child;
    int nread;
    char buf[BUFFERSIZE];

    if ((child = fork()) < 0)
        err_sys("fork error");
    else if (child == 0) { // child copy stdin to ptym
        for (;;) {
            if ((nread = read(STDIN_FILENO, buf, BUFFERSIZE)) < 0)
                err_sys("read error from stdin");
            else if (nread == 0) // EOF on stdin means we're done
                break;
            if (writen(ptym, buf, nread) != nread)
                err_sys("writen faield");
        }

        // 通知父进程
        if (ignoreeof == 0)
            kill(getppid(), SIGTERM);
        exit(0); // terminate; child process can't return
    }
    
    // else { // parent
    // }
    // parent copies ptym to stdout
    if (signal(SIGTERM, sig_term_handler) == SIG_ERR)
        err_sys("signal_inr error on SIGTERM");
    for (;;) {
        if ((nread = read(ptym, buf, BUFFERSIZE)) <= 0)
            break;
        if (writen(STDOUT_FILENO, buf, nread) != nread)
            err_sys("writen failed");
    }

    // 1. sig_term_handler() 捕获到 SIG_TERM
    // 2. pty master 读到了 EOF
    // 3. error occur
    if (sigcaught == 0) // tell child if it didn't send us the signal 
        kill(child, SIGTERM);
    // parent return to caller
}

static void sig_term_handler(int signo)
{
    sigcaught = 1;
}

// TODO:
// 通过终端实现一个 AT 命令解析功能
//https://blog.csdn.net/wubin1124/article/details/5155864?spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-1-5155864-blog-41484345.235%5Ev43%5Epc_blog_bottom_relevance_base6&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-1-5155864-blog-41484345.235%5Ev43%5Epc_blog_bottom_relevance_base6&utm_relevant_index=1
