#include "../common/apue.h"
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <mqueue.h>
/***************************************************************
 * Macro
 ***************************************************************/
#define UDS_SERVER_PATH "/tmp/libuv_server.sock"
#define UDS_CLIENT_PATH "/tmp/unix_client.sock"
/***************************************************************
 * Public function
 ***************************************************************/
static char get_next_char(char ch);

/**
 * @brief uds client
 * 
*/
int start_uds_client(void)
{
    uint8_t *p_buff;
    int client_fd;
    int nread, nwrite;
    char char_to_send = 'a';

    printf("client[%d] connect:%s\n", getpid(), UDS_SERVER_PATH);
    client_fd = cli_conn(UDS_SERVER_PATH);
    ASSERT(client_fd > 0);
    printf("client:fd:%d\n", client_fd);

    p_buff = (uint8_t*)malloc(2 * KB);
    while (1) {
        // 客户端连上后先写, 然后读
        printf("client[%d] send:%c len:%d\n", getpid(), char_to_send, rand() % (2*KB));
        memset(p_buff, char_to_send, 2*KB);
        nwrite = write(client_fd, p_buff, rand() % (2*KB));
        // if (nwrite != 2*KB) {
        //     printf("write fail:%d\n", nwrite);
        // }

        memset(p_buff, 0, 2*KB);
        nread = read(client_fd, p_buff, 2*KB);
        printf("client[%d] read:%c len:%d\n", getpid(), p_buff[0], nread);

        sleep(1);
        char_to_send = get_next_char(char_to_send);
    }
}

/***************************************************************
 * Private function
 ***************************************************************/
static char get_next_char(char ch)
{
    if (ch >= 'z' - 1)
        return 'a';
    if (ch >= 'Z' - 1)
        return 'A';
    
    return ch + 1;
}

