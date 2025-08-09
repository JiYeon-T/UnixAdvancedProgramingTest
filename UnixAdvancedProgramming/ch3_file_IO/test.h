#ifndef __TEST_H__
#define __TEST_H__

void file_test();
void hollow_test();
void copy_file_test();
void file_exist_test();
void dup_file_test();
void dup2_file_test();
void file_control_test(int argc, char **argv);
void set_fl(int fd, int flags);
void clear_fl(int fd, int flags);
void append_test();


#endif

