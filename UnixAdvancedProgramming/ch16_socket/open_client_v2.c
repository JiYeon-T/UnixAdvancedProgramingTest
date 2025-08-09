/**
 * @fun 服务器作为守护进程运行,服务所有客户端请求
 * 两个无关进程之间如果传递文件描述符
 * 客户端与服务端交互的协议仍然与 v1 相同
*/
#include <sys/uio.h> // struct iovec

#include "ch16.h"
#include  "../ch17_IPC/ch17.h"


/**
 * @fun 客户端将文件路径和打开方式发送到服务端, 等待服务端返回文件描述符
 * @param[in] name
 * @param[in] oflag
 * @ret
*/
int csopen_v2(char *name, int oflag)
{
    int len;
    char buf[12];
    struct iovec iov[3];
    static int csfd = -1;

    if (csfd < 0) {
        if ((csfd = cli_conn(CS_OPEN)) < 0) {
            err_ret("cli_conn error");
            return (-1);
        }
    }
    sprintf(buf, " %d", oflag);

    // client request format: open <pathname> <openflag>
    iov[0].iov_base = CL_OPEN " "; // "open " part
    iov[0].iov_len = strlen(CL_OPEN) + 1;
    iov[1].iov_base = name;
    iov[1].iov_len = strlen(name);
    iov[2].iov_base = buf;
    iov[2].iov_len = strlen(buf) + 1; // null always sent
    len = iov[0].iov_len + iov[1].iov_len + iov[2].iov_len;
    if (writev(csfd, &iov[0], 3) != len) {
        err_ret("writev error");
        return (-1);
    }

    // readback descriptro
    return (recv_fd(csfd, write));
}

