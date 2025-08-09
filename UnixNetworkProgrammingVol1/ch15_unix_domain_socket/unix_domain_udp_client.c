#include "../util/util.h"
#include "../util/unp.h"

#include <string.h> // memcpy
#include <strings.h> // bzero()
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <syslog.h>
#include <unistd.h>
#include <sys/un.h> // Unix domain socket


#define UNIX_DOMAIN_STREAM_TEST     1 // TCP
#define UNIX_DOMAIN_DATAGRAM_TEST   0 // UDP

#define UNIX_DOMAIN_SOCKET_PATH     "/tmp/test"


static void dg_client(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servaddrlen);



int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_un cliaddr, servaddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

    /**
     * 与 udp 客户不同的是, 当使用 unix 域协议的时候, 我们必须显式 bind 一个路径名到我们的套接字,
     * 这样服务器才会有能回射应答的路径名(udp套接字客户的端口由操作系统自动分配), 我们调用 tmpnam()
     * 获取一个惟一的路径名.
     * 由一个未绑定的 unix 域套接字发送数据报不会隐式的捆绑一个路径名,因此如果省略掉这一步,那么服务器
     * 在 debug_echo() 函数中的 recvfrom 调用将返回一个空的路径名,这个空的路径名将导致调用 sendto()
     * 出错
    */
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    // /tmp/cceSte0K.o: In function `main':
    // /home/qz/code/Unix_Network_Programing/ch15_unix_domain_socket/unix_domain_udp_client.c:45: 
    // warning: the use of `tmpnam' is dangerous, better use `mkstemp'
    strcpy(&cliaddr.sun_path, tmpnam(NULL));
    LOG_I("client tmp path:%s", cliaddr.sun_path);
    Bind(sockfd, (const struct sockaddr*)&cliaddr, sizeof(cliaddr));

    bzero(&servaddr, sizeof(servaddr)); // fill in server's address
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, UNIX_DOMAIN_SOCKET_PATH);

    dg_client(stdin, sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    LOG_I("client %s exit", cliaddr.sun_path);
    exit(0);
}

/**
 * @brief 该函数是协议无关的
 *       问题:客户端没有判断服务器端信息, 所有发往该端口的数据都被认为是服务器的返回
 * 
*/
static void dg_client(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servaddrlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE + 1];

    while (Fgets(sendline, sizeof(sendline), fp) != NULL) {
        // NOTE: 对于 udp 套接字,客户端首次调用 sendto 时,内核会为该套接字分配一个端口
        // 和 tcp client 一样,客户可以显示调用 bind() 不过很少这样做:
        Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servaddrlen);
        // 不关心服务器端地址信息, 后两个参数可以填 NULL
        n = Recvfrom(sockfd, recvline, sizeof(recvline), 0, NULL, NULL);
        recvline[n] = '\0'; // null terminate
        Fputs(recvline, stdout);
    }
}
