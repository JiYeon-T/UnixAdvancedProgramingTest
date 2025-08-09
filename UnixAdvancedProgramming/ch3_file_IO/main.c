#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "test.h"


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");

    // file_test();

    // hollow_test();

    // copy_file_test();

    // file_exist_test();

    // dup_file_test();

    // dup2_file_test();

    // file_control_test(argc, argv);

    append_test();

	printf("Hello, world end with error:%d\n", errno);

    return 0;
}