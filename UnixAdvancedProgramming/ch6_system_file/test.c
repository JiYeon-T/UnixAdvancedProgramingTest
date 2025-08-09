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

/*
 *	POSIX Standard: 9.2.2 User Database Access	<pwd.h>
 */
#include <pwd.h>

/* Declaration of types and functions for shadow password suite.  */
#include <shadow.h>

/*
 *	POSIX Standard: 9.2.1 Group Database Access	<grp.h>
 */
#include <grp.h>

/* All data returned by the network data base library are supplied in
   host order and returned in network order (suitable for use in
   system calls).  */
#include <netdb.h>

/*
 *	ISO C99 Standard: 7.23 Date and time	<time.h>
 */
#include <time.h>

// #include <sys/time.h> 

#include <sys/utsname.h>


/**************************************************
 * @DECLARATION
 **************************************************/
// /etc/passwd
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);
struct passwd *getpwent(void);
void setpwent(void); // 打开文件
void endpwdent(void); // 关闭文件,
// /etc/shadow
struct spwd *getspnam(const char *name);
struct spwd *getspent(void);
void setspent(void);
void endspent(void);
// /etc/group
struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *name);
void setgrent(void);
void endgrent(void);
// 附属组 ID
int getgroups(int gidsetsize, gid_t grouplist[]);
// int setgroups(int ngroups, const gid_t grouplist[]);
int initgroups(const char *username, gid_t basegid); // 需要由超级用户调用
// 其他附属系统数据文件 如:
// /etc/networks, 
// /etc/hosts, 
// /etc/protocols
// 系统标识
int uname(struct utsname *name);
// int gethostname(char *name, int namelen);
// time
// time_t time(time_t calptr);
int clock_gettime(clockid_t clock_id, struct timespec *tsp);
int clock_getres(clockid_t clock_id, struct timespec *tsp);
int clock_settime(clockid_t clock_id, const struct timespec *tsp);
// int gettimeofday(struct timeval *restrict tp, void * restrict tzp);
struct tm *gmttime(const time_t calptr);
struct tm *localtime(const time_t *calptr);
time_t mktime(struct tm *tmptr);
size_t strftime(char * restrict buf, size_t maxsize, const char * restrict format, \
                const struct tm * restrict tmptr);
size_t strftime_l(char * restrict buf, size_t maxsize, const char * restrict foramt, \
                const struct tm * restrict tmptr, locale_t locale);
char *strptime(const char * restrict buf, const char * restrict format, \
                struct tm *restrict tmptr);



// 通过口令文件 /etc/passwd 获取
static struct passwd* getpwname2(const char *name)
{
    struct passwd *ptr;

    setpwent(); // rewind()

    while ((ptr = getpwent()) != NULL) {
        if (strcmp(name, ptr->pw_name) == 0) {
            break; 
        }
    }

    endpwent();

    return ptr;
}

static void print_passwd_info(struct passwd *ptr)
{
    printf("pw_name:%s\n", ptr->pw_name);
    printf("pw_passwd:%s\n", ptr->pw_passwd);
    printf("pw_uid:%u\n", ptr->pw_uid);
    printf("pw_gid:%u\n", ptr->pw_gid);
    printf("pw_gecos:%s\n", ptr->pw_gecos);
    printf("pw_dir:%s\n", ptr->pw_dir);
    printf("pw_shell:%s\n", ptr->pw_shell);
}

void passwd_file_test()
{
    struct passwd *ptr;

    if ((ptr = getpwname2("qz")) == NULL) {
        err_ret("does not find name:%s\n", "qz");
    }

    print_passwd_info(ptr);

    return;
}

static void print_shadow_passwd_info(struct spwd *ptr)
{
    printf("sp_namp:%s\n", ptr->sp_namp);
    printf("sp_pwdp:%s\n", ptr->sp_pwdp);
    printf("sp_lstchg:%ld\n", ptr->sp_lstchg);
    printf("sp_min:%ld\n", ptr->sp_min);
    printf("sp_max:%ld\n", ptr->sp_max);
    printf("sp_warn:%ld\n", ptr->sp_warn);
    printf("sp_inact:%ld\n", ptr->sp_inact);
    printf("sp_expire:%ld\n", ptr->sp_expire);
}

void shadow_passwd_test()
{
    struct spwd *ptr;

    setspent();
    // 没有权限, 打开失败
    if ((ptr = getspnam("qz")) == NULL) {
        err_ret("get shadow failed");
    }
    print_shadow_passwd_info(ptr);
    endspent();
}

static void print_group_info(struct group *ptr)
{
    printf("gr_name:%s\n", ptr->gr_name);
    printf("gr_passwd:%s\n", ptr->gr_passwd);
    printf("gr_gid:%u\n", ptr->gr_gid);
    // printf("gr_mem:%s\n", ptr->gr_mem);
}

void group_database_test()
{
    struct group *ptr;
    
    setgrent();
    if ((ptr = getgrnam("qz")) == NULL) {
        err_ret("get group failed\n");
    }
    print_group_info(ptr);
    endgrent();

    return;
}

static void display_utsname(struct utsname *ptr)
{
    printf("sysname:%s\n", ptr->sysname);
    printf("nodename:%s\n", ptr->nodename);
    printf("release:%s\n", ptr->release);
    printf("version:%s\n", ptr->version);
    printf("machine:%s\n", ptr->machine);
    printf("__domainname:%s\n", ptr->__domainname);
}

void uname_test()
{
    struct utsname info;
    // char hostname[100];

    uname(&info);
    display_utsname(&info);
    // core dump
    // gethostname("qz", 100);
    // printf("hostname:%s\n", hostname);

    return;
}

static void display_time(struct tm * ptr)
{
    printf("tm_sec:%d\n", ptr->tm_sec);
    printf("tm_min:%d\n", ptr->tm_min);
    printf("tm_hour:%d\n", ptr->tm_hour);
    printf("tm_mday:%d\n", ptr->tm_mday);
    printf("tm_mon:%d\n", ptr->tm_mon);
    printf("tm_year:%d\n", ptr->tm_year);
    printf("tm_wday:%d\n", ptr->tm_wday);
    printf("tm_yday:%d\n", ptr->tm_yday);
    printf("tm_isdst:%d\n", ptr->tm_isdst);
}

void time_test()
{
    time_t t;
    struct tm *tmp;
    char buf1[16];
    char buf2[64];

    time(&t); // tiemstamp
    // t = time(NULL);

    printf("time():%lu\n", t);
    tmp = localtime(&t);
    if (tmp == NULL) {
        err_ret("get time() failed");
    }
    display_time(tmp);

    if (strftime(buf1, 16, "time and date:%r, %a %b %d, %Y", tmp) == 0) {
        err_sys("array size too small");
    } else {
        printf("buf1:%s\n", buf1);
    }

    // if (strftime(buf2, 64, "time and date:%r, %a %b %d, %Y", tmp) == 0) {
    if (strftime(buf2, 64, "time and date:%Y年 %m月 %d日 星期%w %p %Z", tmp) == 0) {
        err_sys("array size too small");
    } else {
        printf("buf2:\n%s\n", buf2);
        system("date");
    }

    tmp = (struct tm *)malloc((sizeof(struct tm) * 1));
    if (tmp == NULL) {
        err_ret("malloc failed");
    }

    strcpy((char*)tmp, buf2);

    // if ( strptime(buf2, "time and date:%r, %a %b %d, %Y", tmp) == NULL) {
    if (strptime(buf2, "time and date:%Y年 %m月 %d日 星期%w %p %Z", tmp) == NULL) {
        err_ret("strptime() failed");
    }
    printf("\n\nreverse:\n");
    display_time(tmp);
    free(tmp);

    exit(0);
}




