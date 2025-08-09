#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "test.h"


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");


    // stdio_test1();
    // stdio_buffer_type_test_2();
    tmp_file_test();
    // tmp_file_test2();
    // memory_stream_test();
    

    if (errno != 0)
	    printf("Hello, world end with error:%d\n", errno);
    else
        printf("Hello, world end\n");

    return 0;
}