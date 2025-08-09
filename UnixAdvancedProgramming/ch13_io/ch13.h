#ifndef __CH13__H__
#define __CH13__H__

#include <stdio.h>


void daemon_process_test(void);
int alread_runing(void);
void io_test1(void);
void file_record_test1(void);
void file_lock_test(int argc, char *argv[]);
void char_process_test(int argc, char *argv[]);
void aio_test1(int argc, char *argv[]);
void aio_read_test1(void);
void aio_suspend_test(void);
void aio_lio_listio_test(void);
void aio_signal_test(void);
void aio_signal_thread_test(void);
void mmap_io_test(int argc, char *argv[]);
void pipe_test1(void);
void pipe_test2(int argc, char *argv[]);
void TELL_WAIT(void);
void TELL_PARENT(pid_t pid);
void WAIT_PARENT(void);
void TELL_CHILD(pid_t pid);
void WAIT_CHILD(void);
void parent_child_sync_test1(void);
void pager_test2(int argc, char *argv[]);
FILE *popen_m(const char *cmdstring, const char *type);
int pclose(FILE *fp);
void popen_test1(void);
void coprocess_test1(void);
void pselect_test(void);


void network_ipc_test1(void);
void get_addr_info_test1(int argc, char *argv[]);
int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen);
int connect_retry_v2(int domain, int type, int protocol, const struct sockaddr *addr, socklen_t alen);
int init_server(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
int init_server_v2(int type, const struct sockaddr *addr, socklen_t alen, int qlen);

#endif