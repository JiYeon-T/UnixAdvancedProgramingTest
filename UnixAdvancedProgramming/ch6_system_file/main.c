#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "test.h"


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");

    // passwd_file_test();
    // shadow_passwd_test();
    // group_database_test();
    // uname_test();
    time_test();

    if (errno != 0)
	    printf("Hello, world end with error:%d\n", errno);
    else
        printf("Hello, world end\n");

    return 0;
}