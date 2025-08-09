#ifndef __CH16_H__
#define __CH16_H__

#include <errno.h>

#include "../common/apue.h"

// int start_ruptime_clent_v1(int argc, char *argv[]);

/************************ open client v1 ***********************************/
// request format:open <pathname> <openmode> \0
#define CL_OPEN     "open"  // client's request for server

int cs_open(char *, int);
int start_open_client_v1(int argc, char *argv[]);
/************************ open server v2 ***********************************/
extern char errmsg[MAXLINE];
extern int oflag;
extern char *pathname;

int start_open_server_v1(int argc, char *argv[]);
int cli_args(int ,char **);
// void handle_request(char *, int, int);
int buf_args(char *buf, int (*optfunc)(int, char **));
int cli_args(int argc, char **argv);


/************************ open client v2 ***********************************/
#define CS_OPEN     "/tmp/opend.socket"     // 服务端众所周知的名字

/************************ open client v2 ***********************************/
extern int debug; // nonzero if interactive(not daemon)
extern char errmsg[];
extern int oflag;
extern char *pathname; // of file to open for client

typedef struct { // one client struct per connected client
    int fd; // fd
    uid_t uid;
} Client;

extern Client *client;      // ptr to malloc'ed array
extern int client_size;     // entries in client[] array

// int cli_args(int, char **);
int client_add(int, uid_t);
void client_del(int);
void loop(void);
void handle_request(char *, int, int, uid_t);


#endif