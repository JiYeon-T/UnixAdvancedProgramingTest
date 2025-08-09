#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"
#include <netinet/sctp.h>

#include <sys/socket.h>

// TODO 这里实现的是 一到多式 sctp, 一到一该如何实现? TODO:与 tcp 类似,但是要使用 sctp_connectx 之类的 api;
// 2. sendmsg() + 辅助数据实现 sctp 服务器客户端程序


#define ECHO_SERV_PORT                  9999 // echo 服务测试端口
#define ECHO_SERV_SCTP_DEBUG            1
#define ECHO_SERV_SCTP_LIMIT_STREAM_NUM 1 // 控制 SCTP 流的数量
#define SERV_MAX_SCTP_STREAM            10 // 流的数量
#define ECHO_SERV_CLOSE_CONNECTION      0 // 服务器 echo 处理结束后主动关闭 sctp 连接,而不是依赖客户端关闭
#define SCTP_NOTIFICATION_DEBUG         1 // 订阅所有的 sctp 通知进行测试
#define SCTP_SIMULATE_TCP               0 // 一对一,
#define SCTP_SIMULATE_UDP               1 // 一对多

#if SCTP_SIMULATE_UDP
int main(int argc, char *argv[])
{
    int sockfd, msg_flags;
    char readbuff[BUFFERSIZE];
    struct sockaddr_in servaddr, cliaddr;
    struct sctp_sndrcvinfo sri;
    struct sctp_event_subscribe events;
    int stream_increment = 1;
    socklen_t addr_len;
    size_t rd_size;

    // 通过传入的参数决定服务端是否增加 接收到消息的流号
    // 可以用于解决 tcp 的头部阻塞问题
    if (argc == 2)
        stream_increment = atoi(argv[1]);
    
    // create a SCTP socket
    // 一到一式套接字:
    // sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    // 一到多式套接字:SOCK_SEQPACKET
    sockfd = Socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);

    // bind wildcard address to the socket
    Bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr));

    /* SCTP 通知:SCTP 为应用程序提供了多种可用的通知 */
    bzero(&events, sizeof(events));
    events.sctp_data_io_event = 1; // subscribe interested SCTP events, 默认情况下,仅 sctp_data_io_event 是开启的
#if SCTP_NOTIFICATION_DEBUG
    events.sctp_adaptation_layer_event = 1; // 
    events.sctp_address_event = 1;
    events.sctp_association_event = 1;
    events.sctp_authentication_event = 1;
    events.sctp_partial_delivery_event = 1;
    events.sctp_peer_error_event = 1;
    events.sctp_send_failure_event = 1;
    events.sctp_sender_dry_event = 1;
    events.sctp_shutdown_event = 1;
#endif
    Setsockopt(sockfd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events));

#if ECHO_SERV_SCTP_LIMIT_STREAM_NUM
    // 控制 sctp 流的数量
    // NOTE: 设置 sctp 流的数量的另一种方法是 使用 sendmsg 函数并提供辅助数据以请求不同于默认设置的流参数, 仅可以用于一到多式套接字.
    struct sctp_initmsg initm;
    bzero(&initm, sizeof(initm));
    initm.sinit_num_ostreams = SERV_MAX_SCTP_STREAM;
    initm.sinit_max_instreams = SERV_MAX_SCTP_STREAM;
    Setsockopt(sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initm, sizeof(initm));
#endif
    Listen(sockfd, LISTENQ);
    LOG_I("SCTP server started");

    for (;;) {
        memset(readbuff, 0, sizeof(readbuff));
        addr_len = sizeof(struct sockaddr_in);
        rd_size = Sctp_recvmsg(sockfd, readbuff, sizeof(readbuff),
                            (const struct sockaddr*)&cliaddr, &addr_len, &sri, &msg_flags);
#if ECHO_SERV_SCTP_DEBUG
        LOG_I("\nsctp sendrecv info:");
        sctp_print_sctp_info(&sri);
        
        if (msg_flags & MSG_NOTIFICATION) { // 接收到的是一个通知,而不是用户数据, 收到通知的数据结构: union sctp_notification
            LOG_I("\nGot Notification:%d rd_size:%d", msg_flags & MSG_NOTIFICATION, rd_size);
            sctp_print_notification((const void *)readbuff);
            continue;
        }
#endif
        if (stream_increment) {
            // sri.sinfo_stream++;
            // TODO:
            // TODO: 怎么获取到 assoc_id ???
            // sctp_get_no_strms get maximum stream number
            // if (sri.sinfo_stream >= sctp_get_no_strms(sockfd,
            //                                         (const struct sockaddr*)&cliaddr,
            //                                         addr_len)) {
            //     sri.sinfo_stream = 0;
            // }
        }
        LOG_I("recv[stream:%d][%d]:%s", sri.sinfo_stream, rd_size, readbuff);
#if ECHO_SERV_CLOSE_CONNECTION
        // 1. sinfo_flags |= MSG_EOF; 迫使所发送消息被客户确认以后,相应关联也终止

        // 2. sinfo_flags |= MSG_ABORT, 迫使立即终止连接,类似于 TCP 的 RSP 分节
        // 能够无延迟的终止任何关联,尚未发送的任何数据都丢弃
        // TODO: 找不到 MSG_EOF 和 MSG_ABORT 定义的地方,
        // 网上说这不是 posix 标准中定义的,不同系统定义不一样.
        // 需要注意的是，并非所有的系统都支持 MSG_EOF。例如，在一些系统（如 Linux）中，MSG_EOF 可能不被支持或者有其替代的实现方式。
        // 在某些情况下，你可以通过发送一个特定的消息（如一个特定的字节序列）来表示 EOF，然后在接收端检测到这个序列来模拟 EOF 的效果。
        // 例如，在 TCP 连接中，通常使用 close() 或 shutdown() 来优雅地关闭连接而不是依赖于 MSG_EOF。
        // 如果你在使用 Linux，并且想要优雅地关闭一个 TCP 连接，可以使用 shutdown() 函数：
        sri.sinfo_flags |= MSG_EOF;
        Sctp_sendmsg(sockfd, readbuff, rd_size, (const struct sockaddr*)&cliaddr,
                    addr_len, sri.sinfo_ppid, sri.sinfo_flags, sri.sinfo_stream, 0, 0);
#else
        Sctp_sendmsg(sockfd, readbuff, rd_size, (const struct sockaddr*)&cliaddr,
                    addr_len, sri.sinfo_ppid, sri.sinfo_flags, sri.sinfo_stream, 0, 0);
#endif
        LOG_I("send[stream:%d][%d]:%s", sri.sinfo_stream, rd_size, readbuff);

#if ECHO_SERV_SCTP_DEBUG
        // LOG_I("sctp send info:");
        // sctp_print_sctp_info(&sri);
#endif

    }

    
}
#endif /* SCTP_SIMULATE_UDP */

#if SCTP_SIMULATE_TCP // 一对一 SCTP 套接字
int main(int argc, char *argv[])
{
    LOG_I("sctp one server");

    while (1) {

    }
}
#endif /* SCTP_SIMULATE_TCP */