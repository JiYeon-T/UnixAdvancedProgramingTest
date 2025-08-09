#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "test.h"


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");

    // dir_test(argc, argv);
    // access_test(argc, argv);
    // file_access_test2();
    // change_file_mode_test();
    // unlink_file_test();
    // symbol_link_test();
    // file_time_test();
    // reset_file_access_time_test(argc, argv);
    // word_directory_test();
    // file_dev_type_test(argc, argv);
    binary_and_oct_test();


    if (errno != 0)
	    printf("Hello, world end with error:%d\n", errno);
    else
        printf("Hello, world end\n");

    return 0;
}