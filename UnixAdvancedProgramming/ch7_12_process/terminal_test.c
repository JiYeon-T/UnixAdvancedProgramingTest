#include "../common/apue.h"

// 实现一个简单的终端:
// step1: init 进程自举过程中, 读取 /etc/ttys 文件中, 每一行 fork() 一个进程
// step2: 每个进程 execle("/bin/login", "login", "-p", "username", (char*)0);
// step3: 对用户输入的密码进行判断, getpass(), getpwnam(char *name) 获取口令文件 
// /etc/passwd 中的用户密码; 如果密码正确, 进程返回; 密码错误超过最大次数, 进程退出 exit(-1);
// step4: 登录成功:加载环境变量setenv(), 修改当前工作目录chdir(), 修改进程所有者 chown(), 修改读写权限:用户读+写...

// 1. 通过硬终端实现的 Login 程序;
// 2. 通过网络终端实现的登录服务;