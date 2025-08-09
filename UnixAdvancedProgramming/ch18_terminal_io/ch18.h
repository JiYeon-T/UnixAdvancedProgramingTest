#ifndef __CH18_H__
#define __CH18_H__

#include "../common/apue.h"

void change_term_stat(void);
void term_state_flag(void);
void term_ICANON_test(void);
void term_get_name(void);
void term_speed_test(void);
void tty_test(void);
void strdup_test(void);
void ttyname_test(void);
void getpass_test(void);
int tty_cbreak(int fd);
int tty_raw(int fd);
int tty_reset(int fd);
void tty_atexit(void);
void abnormal_terminal_test(void);
void winsize_test(void);


int ptym_open(char *pts_name, int pts_namesz);
int ptys_open(char *pts_name);
pid_t pty_fork(int *ptrfdm, char *slave_name, int slave_namesz,
                const struct termios *slave_termios, const struct termios *slave_winsize);
void loop2(int ptym, int ignoreeof);
int pty_test(int argc, char *argv[]);

#endif