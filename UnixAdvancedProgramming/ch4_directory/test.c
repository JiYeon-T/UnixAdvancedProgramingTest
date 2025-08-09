#include "test.h"

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/*
 *	POSIX Standard: 5.6 File Characteristics	<sys/stat.h>
 */
#include <sys/stat.h>

/*
 *	POSIX Standard: 6.5 File Control Operations	<fcntl.h>
 */
#include <fcntl.h>

#include <sys/ioctl.h>

#include <sys/time.h> // #include <sys/stat.h>

/*
 *	POSIX Standard: 5.1.2 Directory Operations	<dirent.h>
 */
#include <dirent.h>



#include <limits.h>
#include "../inc/apue.h"


/*************
 * MACRO 
 *************/
#define RWRWRW      (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

/*************
 * DECLARATION 
 *************/
int stat(const char *restrict pathname, struct stat *restrict buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *restrict pathname, struct stat *restrict buf);
int fstatat(int fd, const char *restrict pathname, struct stat * restrict buf, int flag);
// struct stat
//   {
//     __dev_t st_dev;		/* Device.  */
// #ifndef __x86_64__
//     unsigned short int __pad1;
// #endif
// #if defined __x86_64__ || !defined __USE_FILE_OFFSET64
//     __ino_t st_ino;		/* File serial number.	*/
// #else
//     __ino_t __st_ino;			/* 32bit file serial number.	*/
// #endif
// #ifndef __x86_64__
//     __mode_t st_mode;			/* File mode.  */
//     __nlink_t st_nlink;			/* Link count.  */
// #else
//     __nlink_t st_nlink;		/* Link count.  */
//     __mode_t st_mode;		/* File mode.  */
// #endif
//     __uid_t st_uid;		/* User ID of the file's owner.	*/
//     __gid_t st_gid;		/* Group ID of the file's group.*/
// #ifdef __x86_64__
//     int __pad0;
// #endif
//     __dev_t st_rdev;		/* Device number, if device.  */
// #ifndef __x86_64__
//     unsigned short int __pad2;
// #endif
// #if defined __x86_64__ || !defined __USE_FILE_OFFSET64
//     __off_t st_size;			/* Size of file, in bytes.  */
// #else
//     __off64_t st_size;			/* Size of file, in bytes.  */
// #endif
//     __blksize_t st_blksize;	/* Optimal block size for I/O.  */
// #if defined __x86_64__  || !defined __USE_FILE_OFFSET64
//     __blkcnt_t st_blocks;		/* Number 512-byte blocks allocated. */
// #else
//     __blkcnt64_t st_blocks;		/* Number 512-byte blocks allocated. */
// #endif
// #ifdef __USE_XOPEN2K8
//     /* Nanosecond resolution timestamps are stored in a format
//        equivalent to 'struct timespec'.  This is the type used
//        whenever possible but the Unix namespace rules do not allow the
//        identifier 'timespec' to appear in the <sys/stat.h> header.
//        Therefore we have to handle the use of this header in strictly
//        standard-compliant sources special.  */
//     struct timespec st_atim;		/* Time of last access.  */
//     struct timespec st_mtim;		/* Time of last modification.  */
//     struct timespec st_ctim;		/* Time of last status change.  */
// # define st_atime st_atim.tv_sec	/* Backward compatibility.  */
// # define st_mtime st_mtim.tv_sec
// # define st_ctime st_ctim.tv_sec
// #else
//     __time_t st_atime;			/* Time of last access.  */
//     __syscall_ulong_t st_atimensec;	/* Nscecs of last access.  */
//     __time_t st_mtime;			/* Time of last modification.  */
//     __syscall_ulong_t st_mtimensec;	/* Nsecs of last modification.  */
//     __time_t st_ctime;			/* Time of last status change.  */
//     __syscall_ulong_t st_ctimensec;	/* Nsecs of last status change.  */
// #endif
// #ifdef __x86_64__
//     __syscall_slong_t __glibc_reserved[3];
// #else
// # ifndef __USE_FILE_OFFSET64
//     unsigned long int __glibc_reserved4;
//     unsigned long int __glibc_reserved5;
// # else
//     __ino64_t st_ino;			/* File serial number.	*/
// # endif
// #endif
//   };
// 测试文件按实际用户 ID 和 实际组 ID 进行访问权限测试:Write, Read and execute 权限
int access(const char *pathname, int mode);
int faccessat(int fd, const char *pathname, int mode, int flag);
// 屏蔽部分权限位
mode_t unmask(mode_t cmask);
// 修改文件标志位
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *pathname, mode_t mode, int flag); // fd:AT_FDCWD // flag:AT_SYMLINK_NOFOLLOW
// 改变文件所属用户 ID 以及用户组 ID
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int fchownat(int fd, const char *pathname, uid_t owner, gid_t group, int flag); // fd:AT_FDCWD(use current working directory) 
                                                                                // flag:AT_SYMLINK_NOFOLLOW
int lchown(const char *pathname, uid_t owner, gid_t group);
// 改变文件长度
int truncate(const char *pathname, off_t length);
int ftruncate(int fd, off_t length);
// link(), linkat()...
int link(const char *existingpath, const char *newpath); // 创建一个指向现有文件的链接
int linkat(int efd, const char *existingpath, int nfd, const char *newpath, int flag);
int unlink(const char *pathname);
int unlinkat(int fd, const char *pathname, int flag);
int remove(const char *pathname);
// rename
int rename(const char *oldname, const char *newname);
int renameat(int oldfd, const char * oldname, int newfd, const char *newname);
// symbol link
int symlink(const char *actualpath, const char *sympath);
int symlinkat(const char *actualpath, int fd, const char *sympath);
ssize_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsize);
ssize_t readlinkat(int fd, const char *restrict pathname, char * restrict buf, size_t bufsize);
// change access time and modify time
int futimens(int fd, const struct timespec times[2]);
int utimensat(int fd, const char *path, const struct timespec times[2], int flag);
int utimes(const char *pathname, const struct timeval times[2]);
// struct timespec
// {
        // __time_t tv_sec;		/* Seconds.  */
        // __syscall_slong_t tv_nsec;	/* Nanoseconds.  */
// };
// struct timeval
// {
//     __time_t tv_sec;		/* Seconds.  */
//     __suseconds_t tv_usec;	/* Microseconds.  */
// };
// directory operation
int mkdir(const char *pathname, mode_t mode);
int mkdirat(int fd, const char *pathname, mode_t mode);
int rmdir(const char *pathname);
DIR *opendir(const char *pathname);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dp);
int closedir(DIR *dp);
long telldir(DIR *dp);
void seekdir(DIR *dp, long loc);
// chnage work directory
int chdir(const char *pathname);
int fchdir(int fd);
char *getcwd(char *buf, size_t size);





/**
 * @fun: dir or file type test
*/
void dir_test(int argc, char *argv[])
{
    int i;
    struct stat buf;
    char *ptr;

    for(i = 1; i < argc; ++i) {
        printf("%s:", argv[i]);

        if(lstat(argv[i], &buf) < 0) {
            err_sys("error lstat");
            continue;
        }

        if(S_ISREG(buf.st_mode)) // S_ISREG() 在 stat.h 中定义
            ptr = "regular";
        else if(S_ISDIR(buf.st_mode))
            ptr = "directory";
        else if(S_ISCHR(buf.st_mode))
            ptr = "character";
        else if(S_ISBLK(buf.st_mode))
            ptr = "block";
        else if(S_ISFIFO(buf.st_mode))
            ptr = "FIFO";
        else if(S_ISLNK(buf.st_mode))
            ptr = "Link";
        else if(S_ISSOCK(buf.st_mode))
            ptr = "socket";
        else
            ptr = "unknown device type:%d\r\n", buf.st_mode;
        
        printf("%s\n", ptr);
    }

    exit(0);
}

/**
 * @fun: 文件访问权限测试
*/
void access_test(int argc, char *argv[])
{
    if(argc != 2) {
        err_quit("usage:./a.out <pathname>");
    }

    if(access(argv[1], R_OK) < 0) {
        err_ret("read access error for %s", argv[1]);
    } else {
        printf("read access success for %s\r\n", argv[1]);
    }

    if(open(argv[1], O_RDONLY) < 0) {
        err_ret("open %s failed", argv[1]);
    } else {
        printf("open file %s success\r\n", argv[1]);
    }

    exit(0);
}

/**
 * @fun: 创建文件时改变文件访问权限
*/
void file_access_test2()
{
    const char *path1 = "./foo";
    const char *path2 = "./bar";
    // const char *path3 = "./bar2";

    umask(0);
    if(creat(path1, RWRWRW) < 0) {
        err_sys("create file %s failed", path1);
    } else {
        printf("create file %s success\r\n", path1);
    }

    umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(creat(path2, RWRWRW) < 0) {
        err_sys("create file %s failed", path2);
    } else {
        printf("create file %s success\r\n", path2);
    }

    // if(creat(path3, RWRWRW) < 0) {
    //     err_sys("create file %s failed", path3);
    // } else {
    //     printf("create file %s success\r\n", path3);
    // }
    // 必须要手动恢复, 否则后续创建的文件都没有这些权限
    umask(0);

    exit(0);
}

/**
 * @fun: change file mode
*/
void change_file_mode_test()
{
    struct stat statbuf;
    const char *path1 = "./foo";
    const char *path2 = "./bar";
    const char *path3 = "./bar2";

    if(stat(path1, &statbuf) < 0) {
        err_sys("get file:%s stat failed", path1);
    }

    // turn on set-group-ID and turn off group-execute
    if(chmod(path1, (statbuf.st_mode & ~S_IXGRP)) < 0) {
        err_sys("change file:%s mode failed", path1);
    }

    // rw-r--r--
    if(chmod(path2, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
        err_sys("change file:%s mode failed", path2);
    }

    exit(0);
}

void unlink_file_test()
{
    const char *path = "./tempfile.txt";
    const char *newpath = "./newtempfile.txt";
    int fd;

    if((fd = open(path, O_RDWR | O_CREAT)) < 0) {
        err_sys("open file %s failed", path);
    } else {
        printf("open file %s success\r\n", path);
    }

    // if(unlink(path) < 0) {
    //     err_sys("unlink file %s failed", path);
    // } else {
    //     printf("unlink file %s success\r\n", path);
    // }

    // sleep(15);

    // if(rename(path, newpath) < 0) {
    //     err_sys("rename file %s failed", path);
    // } else {
    //     printf("rename file %s success\r\n", path);
    // }
    
    close(fd);

    if(link(path, newpath)) {
        err_sys("link file %s failed", path);
    } else {
        printf("link file %s -> %s success\r\n", path, newpath);
    }

    printf("done!\r\n");

    exit(0);
}

/*
 * @fun: file link test
 */
void symbol_link_test()
{
    const char *path = "./tempfile.txt";
    const char *symbolLinkPath = "./newtempfile.txt";
    char buf[512];
    int fd;

    // create file
    if((fd = open(path, O_RDWR | O_CREAT)) < 0) {
        err_sys("open file %s failed", path);
    } else {
        printf("open file %s success\r\n", path);
    }

    // close file
    if(close(fd) < 0) {
        err_sys("close file %s failed", path);
    } else {
        printf("close file %s success\r\n", path);
    }
    
    // create symbol link symbolLinkPath point to path
    if(symlink(path, symbolLinkPath) < 0) {
        err_sys("symlink file %s failed", path);
    } else {
        printf("symlink file %s -> %s success\r\n", path, symbolLinkPath);
    }

    // AT_FDCWD
    // read link information
    if(readlink(symbolLinkPath, buf, 512) < 0) {
        err_sys("read link file:%s information failed", symbolLinkPath);
    } else {
        printf("read lin file:%s information success\n", symbolLinkPath);
    }

    for(int ix = 0; ix < 512; ++ix) {
        printf("%c", buf[ix]);
    }

}

/**
 * @fun:modify file access time and modify time, change time will automaically change
*/
void file_time_test()
{
    struct timespec newAccessTime, newModifyTime;
    const char *path = "./tempfile.txt";
    int fd;

    // create file
    if((fd = open(path, O_RDWR | O_CREAT)) < 0) {
        err_sys("open file %s failed", path);
    } else {
        printf("open file %s success\r\n", path);
    }

    // change file access time and modification time
    if(futimens(fd, NULL) < 0) {
        err_sys("change file %s time failed", path);
    } else {
        printf("change file %s time success\r\n", path);
    }

    // close file
    if(close(fd) < 0) {
        err_sys("close file %s failed", path);
    } else {
        printf("close file %s success\r\n", path);
    }
}

/**
 * @fun: 获取文件访问时间, 然后截断文件, 然后恢复文件访问时间,
 *       以达到并不修改文件的时间的目的 
*/
void reset_file_access_time_test(int argc, char *argv[])
{
    int i, fd;
    struct stat statbuf;
    struct timespec times[2];

    for(i = 1; i < argc; ++i) {
        // get the file attribute
        if(stat(argv[i], &statbuf) < 0) {
            err_ret("%s:stat failed", argv[i]);
            continue;
        } else {
            printf("file:%s stat get success\n", argv[i]);
        }

        if((fd = open(argv[i], O_RDWR | O_TRUNC)) < 0) {
            err_ret("%s open failed", argv[i]);
            continue;
        } else {
            printf("file:%s open success\n", argv[i]);
        }
        // fetch current time
        times[0] = statbuf.st_atim;
        times[1] = statbuf.st_mtim;
        // reset time
        if(futimens(fd, times) < 0) {
            err_ret("file:%s futimes error", argv[i]);
        } else {
            printf("file:%s reset time success\n", argv[i]);
        }

        close(fd);
    }

    exit(0);
}

/**
 * @fun traverse directory test and calculate how many files int this directory
*/
#define FTW_F       1
#define FTW_D       2
#define FTW_DNR     3
#define FTW_NS      4

// ???? MyFunc 
typedef int MyFunc(const char *, const struct stat *, int);
static MyFunc myfunc;
static long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;
static char *fullpath;
static size_t pathlen;
static int myftw(char *pathname, MyFunc *fun);

void traverse_directory_op_test(int argc, char *argv[])
{
    // const char *pathname = "./testdir";
    // MyFunc myfunc;
    int ret;

    if(argc != 2) {
        err_quit("usage: ftw <starting pathname>");
    }

    ret =  myftw(argv[1], myfunc);
    ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
    if(ntot == 0) {
        ntot = 1;
    }
    printf("nreg:%ld\nndir:%ld\nnblk:%ld\nnchr:%ld\nnsfifo:%ld\nnslink:%ld\nnsock:%ld\n",\
            nreg, ndir, nblk, nchr, nfifo, nslink, nsock);
}

// we return whatever function return
static int myftw(char *pathname, MyFunc *fun)
{
    // path_alloc()?
    // fullpath = path_alloc()
}

static int dopath(MyFunc *func)
{
    struct stat statbuf;
    struct dirent *pdir;
    DIR           * dp;
    int ret, n;

    if (lstat(fullpath, &statbuf) < 0) {
        return func(fullpath, &statbuf, FTW_NS);
    }

    if (S_ISDIR(statbuf.st_mode) == 0) { // not a directory
        return (func(fullpath, &statbuf, FTW_F));
    }

    // directory
    if ((ret == func(fullpath, &statbuf, FTW_D)) != 0) {
        return ret;
    }

    n = strlen(fullpath);
    if (n + NAME_MAX + 2 > pathlen) { // expand buffter
        pathlen *= 2;
        if ((fullpath = realloc(fullpath, pathlen)) == NULL) {
            err_sys("realloc failed\n");
        }
    }

    fullpath[n++] = '/';
    fullpath[n] = 0;

    if ((dp = opendir(fullpath)) == NULL) { // can't read
        return (func(fullpath, &statbuf,  FTW_DNR));
    }
    while ((pdir = readdir(dp)) != NULL) {
        if (strcmp(pdir->d_name, ".") == 0 ||
            strcmp(pdir->d_name, "..") == 0) {
            continue;
        }
        strcpy(&fullpath[n], pdir->d_name); // append name after
        if ((ret = dopath(func)) != 0) { // recursive
            break; // time to leave
        }
    }

    fullpath[n-1] = 0;
    if(closedir(dp) < 0) {
        err_dump("Can't close directory:%s", fullpath);
    }

    return ret;
}

static int myfunc(const char *pathname, const struct stat *statptr, int type)
{
    switch (type) {
        case FTW_F:
            switch (statptr->st_mode) {
                case S_IFREG:   nreg++; break;
                case S_IFBLK:   nblk++; break;
                case S_IFCHR:   nchr++; break;
                case S_IFIFO:   nfifo++;break;
                case S_IFLNK:   nslink++;break;
                case S_IFSOCK:  nsock++; break;
                case S_IFDIR: // directory should have FTW_D
                    err_sys("error pathname:%s\n", pathname);
            }
            break;
        case FTW_D:
            ndir++;
            break;
        case FTW_DNR:
            err_sys("can't read directory:%s", pathname);
            break;
        case FTW_NS:
            err_sys("stat error for %s", pathname);
            break;
        default:
            err_sys("unknown type:%d for path name:%s", type, pathname);
    }

    return 0;
}

// 当前工作目录是进程的一个属性, 进程调用 chdir(), chdir() 可以修改进程的当前工作目录
// NOTE:只影响调用进程的工作目录, 不影响其他进程的工作目录
// 起始目录则是登录名的一个属性
void word_directory_test()
{
    char * workDir = "/tmp";
    char buff[NAME_MAX];

    // printf("%d\n", system("pwd"));
    // system("pwd");
    getcwd(buff, NAME_MAX);
    printf("current work directory:%s\n", buff);

    if (chdir(workDir) < 0) {
        err_sys("chdir failed");
    }

    printf("chdir preocess workd dir to:%s success\n", workDir);
    // printf("curr dir:%s\n", system("pwd"));
    // system("pwd");
    getcwd(buff, NAME_MAX);
    printf("current work directory:%s\n", buff);

    return;
}

/**
 * @fun: device number, struct stat.st_dev, st_rdev
*/
void file_dev_type_test(int argc, char *argv[])
{
    int i;
    struct stat buf;

    for (i = 1; i < argc; ++i) {
        printf("%s\n", argv[i]);
        if (stat(argv[i], &buf) < 0) {
            err_sys("get file:%s stat filed", argv[i]);
            continue;
        }
        printf("dev = %d/%d", major(buf.st_dev), minor(buf.st_dev));

        // 只有字符设备和块设备才有 st_rdev 值
        if (S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode)) {
            printf("  (%s)  redev = %d/%d\n", S_ISCHR(buf.st_mode) ? "character" : "block", 
                    major(buf.st_rdev), minor(buf.st_rdev));
        }
        printf("\n");
    }

    return;
}

/**
 * @fun: 二进制以及8进制
*/
void binary_and_oct_test()
{
    // 十进制打印
    int a = 5;

    printf("deciaml:%d, hex:%#x, oct:%o, bin:\n", a, a, a);

    // 表示:
    // 0x 表示 16 进制
    // 0 开头表示 8 进制
    // 0b/0B 开始表示二进制
    printf("%d  %d  %d %d\n", 10, 0x0A, 012, 0b1010); //
}