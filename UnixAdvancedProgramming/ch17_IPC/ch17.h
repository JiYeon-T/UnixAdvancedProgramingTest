#ifndef __CH17_H__
#define __CH17_H__

int fd_pipe(int fd[2]);

void socketpair_test(void);
void socketpair_send_test(int argc, char *argv[]);
void unix_socket_test1(void);

void unix_socket_server_test2(void);
void unix_socket_client_test2(void);

void send_fd_server_test1(void);
void recv_fd_client_test1(void);

int start_uds_client(void);

void uds_thread_test(void);


#endif