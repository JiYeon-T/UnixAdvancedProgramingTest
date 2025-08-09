#include <stdio.h>
#include <stdlib.h>
#include "./test.h"


int main(int argc, char **argv)
{
   printf("Hello world!\n");
   for(int ix = 0; ix < argc; ++ix) {
       printf("%s\r\n", argv[ix]);
   }

    // login
    // 口令文件: /etc/passwd
    // 登陆时, 设置工作目录为起始目录, 起始目录可以在口令文件中查到, eg:qz:x:1000:1000:qz,,,:/home/qz:/bin/bash

    //
    // print_hello(NULL);
    // file_operation_test();


    // directory_op_test(argc, argv);

    // input_output_test();
    // input_output_test2();

    // 进程 process, process ID
    // printf("process ID:%ld\r\n", (long)getpid());
    // process_test();

    // get current user ID
    // uid_gid_test();

    // signal test
    signal_test();

    return 0;
}
