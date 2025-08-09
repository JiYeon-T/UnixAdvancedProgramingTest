#ifndef __TEST_H__
#define __TEST_H__

#include <stddef.h>

void dir_test(int argc, char *argv[]);
void access_test(int argc, char *argv[]);
void file_access_test2();
void change_file_mode_test();
void unlink_file_test();
void symbol_link_test();
void file_time_test();
void reset_file_access_time_test(int argc, char *argv[]);
void traverse_directory_op_test();
void word_directory_test();
void file_dev_type_test(int argc, char *argv[]);
void binary_and_oct_test();


#endif