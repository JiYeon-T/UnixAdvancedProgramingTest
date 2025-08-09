
#include <unistd.h>
#include <termios.h>

#include "../common/apue.h"
#include "../common/list.h"

struct s_leader;
struct proc;
struct tty;
struct vnode;
struct pgrp;
struct list;

/**
 * session 的数据结构
*/
struct session {
    int s_count; // 该会话中的进程组数, 减到 0 时, 可以释放此结构
    struct proc *s_leader; // 会话首进程 proc 结构的指针
    struct vnode *s_ttyvp; // 指向控制终端的 vnode 结构的指针
    struct tty *s_ttyp; // 指向控制终端的 tty 结构的指针
    pid_t s_sid; // 会话 ID
};

/**
 * tty 结构
 * 每个终端设备/伪终端设备均在内核中分配一个这样的数据结构
*/
struct tty {
    struct session *t_session; // 指向此控制终端的 session 结构图
    struct proc *t_pgrp; //
    struct termios *t_termios;
    int  t_winsize;
};

/**
 * pgrp 包含一个特定进程组信息
*/
struct pgrp {
    pid_t pg_id;
    struct session *pg_session;
    struct proc *pg_members;
};

/**
 * 进程组成员
*/
struct proc {
    struct list *p_pglist;
    pid_t p_pid;
    struct proc *p_pptr;
    struct pgrp *p_pgrp;
};

/**
 * 内核为每个终端都维护了一个 winsize 结构
 * 窗口大小变化时,内核通知前台进程组
*/
struct winsize {
    unsigned short ws_row; // rows in character
    unsigned short ws_col; // cols in character
    unsigned short ws_xpixel; // horizontal size, pixels(unused)
    unsigned short ws_ypixel; // vertical size, pixels(unused)
};
/**
 * @brief setsid_m FreeBSD 实现(内核实现)
*/
pid_t setsid_m(void)
{
    // 内核中分配一个 session 数据结构
    struct session *p = malloc(sizeof(struct session));
    // p->s_count = 1;
    // p->s_leader = 调用进程 proc 结构的指针
    // p->s_sid = 进程 id;
    // p->s_ttyvp = NULL; // 会话没有控制终端
    // p->s_ttyp = NULL; 
}