#include "../common/apue.h"

#include <stdio.h>
#include <stdarg.h>
/*
 *      ISO C99 Standard: 7.24
 *	Extended multibyte and wide character utilities	<wchar.h>
 */
#include <wchar.h>

/*
 *	POSIX Standard: 5.6 File Characteristics	<sys/stat.h>
 */
#include <sys/stat.h>

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/**************************************************
 * @DECLARATION
 **************************************************/
// 修改重定向
int fwide(FILE *fp, int mode);
// 修改系统指定缓冲类型
void setbuf(FILE * restrict fp, char * restrict buf); // size = BUFSIZ -> stdio.h
// void setvbuf(FILE * restrict fp, char * restrict buf, int mode, size_t size);
int fflush(FILE *fp);
//stream operation
FILE *fopen(const char * restrict pathname, const char * restrict type);
FILE *freopen(const char * restrict pathname, const char * restrict type, FILE * restrict fp);
FILE *fdopen(int fd, const char *type);
int fclose(FILE *fp);
// read & write stream  
// extern int ferror (FILE *__stream) __THROW __wur; // 获取 IO 失败原因, ret:EOF
// extern int feof (FILE *__stream) __THROW __wur;
// extern void clearerr (FILE *__stream) __THROW;
// (1) characetr IO; 
int gec(FILE *fp); // getc() 可以被实现为宏
int fgetc(FILE *fp); // fgetc() 不可以被实现为宏
int getchar(void); // stdin
int ungetc(int c, FILE *fp); // 读取数据后, 调用 ugetc() 将字符再送回流中(并没有会送到底层文件或者设备,回送到缓冲区)
int putc(int c, FILE *fp); // fp
int fpuc(int c, FILE *fp);
int puchar(int c); // stdout
// (2)line IO;
char *fgets(char * restrict buf, int n, FILE * restrict fp); // fp, 读入 n-1 个字符, 保存 null
char *gets(char *buf); // stdin, 并不将换行符存入缓冲区, 可能导致缓冲区溢出
int fputs(const char * restrict str, FILE * restrict fp);
int puts(const char *str);
// (3)direct IO; 二进制 IO
size_t fread(void * restrict ptr, size_t size, size_t nobj, FILE  * restrict fp);
size_t fwrite(const void * restrict ptr, size_t size, size_t nobj, FILE * restrict fp);
// 定位流
// fgetpos(), fsetpos()
long ftell(FILE *fp);
int fseek(FILE *fp, long offset, int whence);
void rewind(FILE *fp);
off_t ftello(FILE *fp); // off_t
int fseeko(FILE *fp, off_t offset, int whence);
// 非 UNIX 系统中
// int fgetpos(FILE * restrict fp, fpos_t * restrict pos); // stdio.h
// int fsetpos(FILE * restrict fp, const fpos_t *pos);
// format IO
int printf(const char * restrict format, ...); // stdout
int fprintf(FILE * restrict fp, const char * restrict format, ...); // fp, write to specified stream
int dprintf(int fd, const char * restrict format, ...); // fd, write to specified file descriptor
int sprintf(char * restrict buf, const char * restrict format, ...); // string
int snprintf(char * restrict buf, size_t n, const char * restrict format, ...); // add size
// extern FILE *fdopen (int __fd, const char *__modes) __THROW __wur; // 将文件描述符转化为文件指针
// int fileno(FILE *fp); // 文件指针转换为文件描述符
// 可变参数表 va_list 替换 ...
int vprintf(const char * restrict format, va_list arg);
int vfprintf(FILE * restrict fp, const char * restrict format, va_list arg);
int vdprintf(int fd, const char * restrict format, va_list arg);
int vsprintf(char * restrict buf, const char * restrict format, va_list arg);
int vsnprintf(char * restrict buf, size_t n, const char * restrict format, va_list arg);
// inputs
int scanf(const char * restrict format, ...); // stdin
int fscanf(FILE * restrict fp, const char * restrict format, ...); // fp
int sscanf(const char * restrict fp, const char * restrict format, ...); // string
// va_list
int vscanf(const char * restrict format, va_list arg);
int vfscanf(FILE * restrict fp, const char * restrict format, va_list arg);
// int vsscanf(char * restrict buf, const char * restrict cormat, va_list arg);
// 通过 FILE *fp -> fd, 标准 IO 库最终都要调用系统调用 System Call
int fileno(FILE *fp);
// temporary file
char *tmpnam(char *ptr); // 有风险其他进程可能也正在使用这个函数
FILE *fmpfile(void); // 会删除
char *mkdtemp(char *tempplate);
// char *mkstemp(char *tempplate);
// 内存流
FILE *fmemopen(void * restrict buf, size_t size, const char * restrict type);
FILE *open_memstream(char **bufp, size_t *sizep); // 字节流
FILE *open_wmemstream(char **bufp, size_t *sizep); // 宽字节流


/**************************************************
 * @TYPEDEF
 **************************************************/
typedef struct {
    char name[20];
    int score[10];
    int id;
} test_struct_t;

/**************************************************
 * @GLOBAL FUNCTIONS
 **************************************************/
void stdio_test1()
{
    char ch;
    int ret;
    char buff[MAXLINE] = {0};
    char outputbuff[MAXLINE] = {0};
    char *path = "./test_struct.bin";
    FILE *fp;

    // character IO
    // while ((ch = getc(stdin)) != EOF) {
    //     if (putc(ch, stdout) == EOF) {
    //         err_sys("puchar error");
    //     }
    // }
    // if (ferror(stdin)) {
    //     err_sys("input error\n");
    // }
    
    // 设置为行缓冲后为什么会多输出一个字符——谨慎使用
    // if (setvbuf(stdout, outputbuff, _IOLBF, MAXLINE) != 0) {
    //     err_ret("set line buffer failed");
    // }    
    // while (fgets(buff, 10, stdin) != NULL) {
    //     printf("\noutput:\n");
    //     // if (fputs(buff, stdout) == EOF) {
    //     //     err_sys("output error");
    //     // }
    //     for (int ix = 0; ix < strlen(buff); ++ix) {
    //         putc(buff[ix], stdout);
    //     }
    //     memset(buff, 0, MAXLINE);
    // }

    // binary IO
    fp = fopen(path, "a+b");
    if (!fp) {
        err_ret("open file:%s failed", path);
    }

    printf("struct size:%ld\n", sizeof(test_struct_t));
    test_struct_t test = {"Lebron james", {1, 2, 3, 4, 5}, 12345};
    if (fwrite(&test, sizeof(test_struct_t), 1, fp) != 1) {
        err_sys("write file:%s failed", path);
    }

    // reset
    fseek(fp, 0, SEEK_SET);
    memset(&test, 0, sizeof(test));

    if (fread(&test, sizeof(test_struct_t), 1, fp) == 1) {
        printf("name:%s\n", test.name);
        printf("name:%d\n", test.id);
        printf("name:%d %d\n", test.score[0], test.score[1]);
    } else {
        err_sys("read file:%s failed", path);
        err_sys("error:%d", ferror(fp));
    }

    fclose(fp);

    if (ferror(stdin)) {
        err_sys("input error");
    }

    return;
}

void format_io_test()
{

}

void pr_stdio(const char *name, FILE *fp);
int is_unbuffered(FILE *fp);
int is_linebuffered(FILE *fp);
int buffer_size(FILE *fp);

void pr_stdio(const char *name, FILE *fp)
{
    printf("stream = %s, ", name);

    if (is_unbuffered(fp)) {
        printf("unbuffered\n");
    } else if (is_linebuffered(fp)) {
        printf("line buffered\n");
    } else {
        printf("full buffered\n");
    }
    printf("buffer size:%d\n", buffer_size(fp));

    return;
}

#if defined(_IO_UNBUFFERED)
int is_unbuffered(FILE *fp)
{
    return (fp->_flags & _IO_UNBUFFERED);
}

int is_linebuffered(FILE *fp)
{
    return (fp->_flags & _IO_LINE_BUF);
}

int buffer_size(FILE *fp)
{
    return (fp->_IO_buf_end - fp->_IO_buf_base);
}
#elif defined(__SNBF)
int is_unbuffered(FILE *fp)
{
    return (fp->_flags & __SNBF);
}

int is_linebuffered(FILE *fp)
{
    return (fp->_flags & __SLBF);
}

int buffer_size(FILE *fp)
{
    return (fp->_bf._size);
}
#elif defined(_IONBF)

#ifdef _LP64
#define _flag __pad[4]
#define _ptr __pad[1]
#define _base __pad[2]
#endif

int is_unbuffered(FILE *fp)
{
    return (fp->_flags & _IONBF);
}

int is_linebuffered(FILE *fp)
{
    return (fp->_flags & _IOLBF);
}

int buffer_size(FILE *fp)
{
#ifdef _LP64
    return (fp->_base - fp->_ptr;
#else
    return (BUFSIZ);
#endif
}

#else
#error "unknown stdio implentation"
#endif

void stdio_buffer_type_test_2()
{
    FILE *fp;
    const char *path = "./test.txt";

    fputs("enter any character:\n", stdout);
    if (getchar() == EOF) {
        err_sys("getchar() failed");
    }

    pr_stdio("stdin", stdin);
    pr_stdio("stdout", stdout);
    pr_stdio("stderr", stderr);

    fp = fopen(path, "r+w");
    if (!fp) {
        err_ret("open file:%s failed", path);
    }

    // 必须要进行一次 IO 才会分配缓冲区, 否则缓冲区大小就是 0
    if (getc(fp) == EOF) {
        err_sys("getchar failed");
    }

    pr_stdio("fp", fp);

    if (fclose(fp) != 0) {
        err_ret("close file:%s failed, errno:%d", path, errno);
    }

    return;
}

void tmp_file_test()
{
    char name[L_tmpnam], line[MAXLINE];
    FILE *fp;

    printf("tmpname():%s\n", tmpnam(NULL));
    tmpnam(name);
    printf("tmpname:%s\n", name);

    fp = tmpfile();
    if (!fp) {
        err_ret("create tmp file failed\n");
    }

    fputs("oneline of output:\n", fp);
    rewind(fp);
    if (fgets(line, sizeof(line), fp) == NULL) { // read back
        err_ret("fgets() failed\n");
    }
    fputs(line, stdout);

    return;
}

static void make_temp(char *template)
{
    int fd;
    struct stat statbuf;

    if ((fd = mkstemp(template)) < 0) {
        err_ret("create temp file faile");
    }
    printf("temp file name:%s\n", template);
    close(fd);

    if (stat(template, &statbuf) < 0) {
        if (errno == ENOENT) {
            err_sys("file:%s does not exist", template);
        } else {
            err_sys("get file:%s stat failed", template);
        }
    } else {
        printf("file:%s exist\n", template);
        unlink(template);
    }

    return;
}

void tmp_file_test2()
{
    // 正确的方式, 分配栈上内存, 因为这部分内存函数 mkstemp() 要先修改
    char good_template[] = "/tmp/dirXXXXXXXX";
    // 不好的方式, 静态区内存, 函数无法修改
    char *bad_template = "/tmp/dirXXXXXXXX";

    make_temp(good_template);
    // make_temp(bad_template); // core dump
    exit(0);
}

// 内存流
void memory_stream_test()
{
    #define SIZE    20
    FILE *fp;
    char buf[SIZE];

    memset(buf, 'a', SIZE - 2);
    buf[SIZE - 2] = '\0';
    buf[SIZE - 1] = 'X';
    
    if ((fp =fmemopen(buf, SIZE, "w+")) == NULL) {
        err_ret("fmemopen failed\n");
    }

    printf("init success:buf:%s\n", buf);
    fprintf(fp, "Hello, world\n"); // 写到内存流
    printf("before flush buf:%s len:%lu\n", buf, strlen(buf));
    fflush(fp);
    printf("after fflush buf:%s len:%lu\n\n", buf, strlen(buf));

    memset(buf, 'b', SIZE - 2);
    buf[SIZE - 2] = '\0';
    buf[SIZE - 1] = 'X';
    fprintf(fp, "Hello, world");
    fseek(fp, 0, SEEK_SET);
    printf("after seek buf:%s len:%lu\n\n", buf, strlen(buf));

    memset(buf, 'c', BUFSIZ - 2);
    buf[SIZE - 2] = '\0';
    buf[SIZE - 1] = 'X';
    fprintf(fp, "Hello, world");
    fclose(fp);
    printf("after fclose buf:%s len:%lu\n\n", buf, strlen(buf));

    return;
}
















// TODO:
// 1. vsscanf() 等以 v 开头的函数的使用, 形参: va_list arg;
// 2. scanf(), [ [^  的使用
// 3. 可变参数 ... 与 va_list 的使用有什么区别


