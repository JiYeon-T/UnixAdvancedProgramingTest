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


void str_cli(int input_fd, int sockfd);




/**
 * @brief TCP/数据流套接字, 提供没有边界的字节流服务
 * 
*/
int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_un servaddr;

    sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, UNIX_DOMAIN_SOCKET_PATH);

    if (connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
        err_ret("connect failed");
    LOG_I("connect to [%d]", sockfd);

    // str_cli(stdin, sockfd); // read from stdin and send to server
    str_cli(STDIN_FILENO, sockfd); // read from stdin and send to server
    LOG_I("client [%d] exit", sockfd);
    exit(0);
}

void str_cli(int input_fd, int sockfd)
{
    int n;
    char buff[MAXLINE];

    for (; ;) {
        if ( (n = Read(input_fd, buff, sizeof(buff))) <= 0 )
            break;
        buff[n] = '\0';
        Write(sockfd, buff, strlen(buff));
        bzero(buff, sizeof(buff));
        if ( (n = Read(sockfd, buff, sizeof(buff))) <= 0 )
            break;
        LOG_I("read from server:%s", buff);
    }
}