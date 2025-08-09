#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"

#include <netinet/sctp.h>

#include <sys/socket.h>

#define ECHO_SERV_PORT                  9999 // echo 服务测试端口
#define SERV_MAX_SCTP_STREAM            10
#define ECHO_CLIENT_CLOSE_CONNECTION    0 // 客户端发送完数据后主动哦该断开连接, Linux 不支持???
#define SCTP_NOTIFICATION_DEBUG         0 // 订阅所有的 sctp 通知进行测试
#define SCTP_SIMULATE_TCP               0 // 一对一, TODO:
#define SCTP_SIMULATE_UDP               1 // 一对多, 当前代码

static void sctp_str_cli(FILE *fp, int sock_fd, struct sockaddr *to, socklen_t tolen);
static void sctp_str_cli_echoall(FILE *fp, int sock_fd, struct sockaddr *to, socklen_t tolen);

#if SCTP_SIMULATE_UDP
// 一对多方式:
// ./client 127.0.0.1
int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_in servaddr;
    struct sctp_event_subscribe events;
    int echo_to_all = 0;

    if (argc < 2) {
        err_quit("Missing host argument, usage '%s <host_ip> [echo_num]\n'", argv[0]);
    }
    if (argc > 2) {
        LOG_I("Echo message to all streams");
        echo_to_all = 1;
    }

    sock_fd = Socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(ECHO_SERV_PORT);
    Inet_pton(AF_INET, argv[1], &servaddr.sin_addr); // argv[1] is remote address

    // 设置套接字选项,预订感兴趣的通知
    bzero(&events, sizeof(events));
    events.sctp_data_io_event = 1;
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
    Setsockopt(sock_fd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events));

    LOG_I("echo_to_all:%d", echo_to_all);
    /**
     * 对于一到一式套接字:客户必须首先调用 sctp_connect 显式建立关联,但是该函数没有额外指定数据的参数,因此无法在四路握手
     * 的第三个分组中随 COOKIE ECHO 消息携带数据.
     * 对于一到多式套接字:客户不必先建立关联再发送数据,而是可以调用 sctp_sendto 同时完成 2 者,也就是说这样发送的数据随
     * COOKIE ECHO 发送到对端,这样的关联是隐式建立的
    */
    if (echo_to_all == 0) {
        sctp_str_cli(stdin, sock_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    } else {
        sctp_str_cli_echoall(stdin, sock_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }

#if ECHO_CLIENT_CLOSE_CONNECTION
    // 1. sinfo_flags |= MSG_EOF; 迫使所发送消息被客户确认以后,相应关联也终止

    // 2. sinfo_flags |= MSG_ABORT, 迫使立即终止连接,类似于 TCP 的 RSP 分节
    // 能够无延迟的终止任何关联,尚未发送的任何数据都丢弃
    // 客户端主动断开 SCTP 连接:
    // 需要注意的是，并非所有的系统都支持 MSG_EOF。例如，在一些系统（如 Linux）中，MSG_EOF 可能不被支持或者有其替代的实现方式。
    // 在某些情况下，你可以通过发送一个特定的消息（如一个特定的字节序列）来表示 EOF，然后在接收端检测到这个序列来模拟 EOF 的效果。
    // 例如，在 TCP 连接中，通常使用 close() 或 shutdown() 来优雅地关闭连接而不是依赖于 MSG_EOF。
    // 如果你在使用 Linux，并且想要优雅地关闭一个 TCP 连接，可以使用 shutdown() 函数：
    char byemsg[32];
    strcpy(byemsg, "goodbye");
    Sctp_sendmsg(sock_fd, byemsg, streln(byemsg),
                (const struct sockaddr *)&servaddr, sizeof(servaddr), 0,
                MSG_ABORT, 0, 0, 0);
#endif
    Close(sock_fd);

    return (0);
}

#endif /* SCTP_SIMULATE_UDP */

// 仅测试使用
// union sctp_notification 类型参数解析,联合体,联合体中保存了各种不同类型
void sctp_notif_param_parse(void *notif)
{
    union sctp_notification *p = (union sctp_notification*)notif;
    switch (p->sn_header.sn_type) {
        case SCTP_ASSOC_CHANGE:

        break;
        case SCTP_PEER_ADDR_CHANGE:
        break;
        case SCTP_REMOTE_ERROR:
        break;
        case SCTP_SEND_FAILED:
        break;
        case SCTP_SHUTDOWN_EVENT:
        break;
        case SCTP_ADAPTATION_INDICATION:
        break;
        case SCTP_PARTIAL_DELIVERY_EVENT:
        break;
        default:
        break;
    }
}

/**
 * @brief
 * 
 * @param[in]  fp  数据格式: [0]hello
 * @param[in]  sock_fd
 * @param[in]  to
 * @param[in]  tolen
*/
static void sctp_str_cli(FILE *fp, int sock_fd, struct sockaddr *to, socklen_t tolen)
{
    struct sockaddr_in peeraddr;
    struct sctp_sndrcvinfo sri;
    char sendline[MAXLINE], recvline[MAXLINE];
    socklen_t addr_len;
    int out_sz, rd_sz;
    int msg_flags;

    bzero(&sri, sizeof(sri));
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        // 数据交互格式,如: [0]hello,world, 中间数字为 stream_id
        if (sendline[0] != '[') {
            LOG_E("line must be the form '[stream_no] text'");
            continue;
        }

        sri.sinfo_stream = strtol(&sendline[1], NULL, 0); // 流号
        out_sz = strlen(sendline);
        memset(recvline, 0, sizeof(recvline));
        msg_flags = 0;
        Sctp_sendmsg(sock_fd, sendline, out_sz, to, tolen, 0, 0, sri.sinfo_stream, 0, 0);

        addr_len = sizeof(peeraddr);
        rd_sz = Sctp_recvmsg(sock_fd, recvline, sizeof(recvline), 
                            (const struct sockaddr *)&peeraddr, &addr_len,
                            &sri, &msg_flags);
        if (rd_sz < 0) {
            LOG_E("remote server closed error:%s", strerror(errno));
            break;
        }
        LOG_I("From stream:%d seq:%d (assoc:0x%x)", sri.sinfo_stream,
            sri.sinfo_ssn, sri.sinfo_assoc_id);
#if SCTP_NOTIFICATION_DEBUG
        // Q: 为什么客户端收不到通知???
        // A: 因为没有订阅
        // TODO: fgets() 和 sctp_recvmsg 同时阻塞导致接收到的数据打印表示实时的,本次打印的是上次接收到的值
        // 需要使用 IO 多路复用
        if (msg_flags & MSG_NOTIFICATION) {
            sctp_print_notification((const void *)recvline);
            continue;
        }
#endif
        LOG_I("recv[%d]:%s", rd_sz, recvline);

        memset(sendline, 0, sizeof(sendline));
    }
}

/**
 * @brief 为了测试 "sctp" 使用不同的流避免头部阻塞问题
 * @note  接收到的数据被发送到所有的 SERV_MAX_SCTP_STREAM 流中,然后接收响应
 * 现象:
 * 1. 当两台主机间的路由器没有丢包和延迟时, 收到数据的顺序和发送一致,
 * 2, 当有丢包/延迟时, 收到的顺序会和发送不一致,因为丢包时当前流会阻塞,但是其他
 * 流号上的数据正常传输. 没有头部阻塞(TCP)问题
*/
static void sctp_str_cli_echoall(FILE *fp, int sock_fd, struct sockaddr *to, socklen_t tolen)
{
    struct sockaddr_in peeraddr;
    struct sctp_sndrcvinfo sri;
    char sendline[MAXLINE], recvline[MAXLINE];
    socklen_t len;
    int out_sz, rd_sz, i, strsz;
    int msg_flags;

    bzero(&sri, sizeof(sri));
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        strsz = strlen(sendline);
        if (sendline[strsz - 1] == '\n') { // 删除末尾的换行符
            sendline[strsz - 1] = '\0';
            strsz--;
        }

        for (i = 0; i < SERV_MAX_SCTP_STREAM; i++) {
            // 为发送的添加后缀,便于调试/分析
            snprintf(sendline + strsz, sizeof(sendline) - strsz, "(msg_id:%d)", i);
            Sctp_sendmsg(sock_fd, sendline, sizeof(sendline), to, tolen, 0, 0, i, 0, 0);
            LOG_I("send[stream:%d][%d]:%s", i, sizeof(sendline), sendline);
        }

        for (i = 0; i < SERV_MAX_SCTP_STREAM; i++) {
            len = sizeof(peeraddr);
            memset(recvline, 0, sizeof(recvline));
            rd_sz = Sctp_recvmsg(sock_fd, recvline, sizeof(recvline), 
                                (const struct sockaddr *)&peeraddr, &len,
                                &sri, &msg_flags);
            LOG_I("From stream:%d seq:%d (assoc:0x%x)", sri.sinfo_stream,
                sri.sinfo_ssn, sri.sinfo_assoc_id);
            LOG_I("recv[%d][%d]:%s", sri.sinfo_stream, rd_sz, recvline);
        }

        memset(sendline, 0, sizeof(sendline));
    }
}

#if SCTP_SIMULATE_TCP // 一对一 SCTP 套接字
int main(int argc, char *argv[])
{
    LOG_I("sctp one client");

    while (1) {
        
    }
}
#endif /* SCTP_SIMULATE_TCP */