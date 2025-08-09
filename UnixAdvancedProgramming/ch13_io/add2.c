#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>


// 使用底层 IO(UNIX 系统调用)
int main(int argc, char *argv[])
{
    int n, int1, int2;
    char line[512];

    while ((n = read(STDIN_FILENO, line, 1024)) > 0) {
        line[n] = 0;
        if ((sscanf(line, "%d%d", &int1, &int2)) == 2) {
            sprintf(line, "%d\n", int1 + int2);
            n = strlen(line);
            if (write(STDOUT_FILENO, line, n) != n)
                printf("'write err'");
        } else {
            if (write(STDOUT_FILENO, "Invalid args\n", 13) != 13)
                printf("'write err'");
        }
    }

    exit(0);
}

// 当使用标准库接口接口时(stdin, stdout) 由于子线程的标准输入为管道,默认为全缓冲, 则需要修改缓冲类型为行缓冲
// int main(int argc, char *argv[])
// {
//     int int1, int2;
//     char line[512];

//     // 设置行缓冲
//     if (setvbuf(stdin, NULL, _IOLBF, 0) != 0)
//         err_sys("setvbuf error");
//     if (setvbuf(stdout, NULL, _IOLBF, 0) != 0)
//         err_sys("setvbuf error");
//     while (fgets(line, 512, stdin) != NULL) {
//         if ((sscanf(line, "%d%d", &int1, &int2)) == 2) {
//             sprintf(line, "%d\n", int1 + int2);
//             if (fprintf(stdout, "%s\n", line) < 0)
//                 printf("'write err'");
//         } else {
//             if (fprintf(stdout, "%s\n", line) < 0)
//                 printf("'write err'");
//         }
//     }

//     exit(0);
// }