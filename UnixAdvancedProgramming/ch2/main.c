#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "test.h"
#include <stdint.h>


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");

	// get_config();

	// size_t pathSize; // error, only for test
	// path_alloc(&pathSize);

	// daemon_process_test();

	times_test();

	printf("Hello, world end with error:%d\n", errno);
}
