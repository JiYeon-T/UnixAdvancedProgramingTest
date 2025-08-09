#include "../util/util.h"
#include "../util/unp.h"
#include "../util/sum.h"


static const char *bool_str(bool flag);
static void print_socket_config_v2(void);
static void print_socket_config_v3(void);


int main(int argc, char *argv[])
{
    // print_socket_config();
    print_socket_config_v2();
    print_socket_config_v3();
}

/**
 * @brief fcntl
*/
static void print_socket_config_v2()
{
    int fd;

    fd = Socket(AF_INET, SOCK_STREAM, 0);

    LOG_D("设置套接字为非阻塞式IO:%s", get_fl(fd, O_NONBLOCK) ? "ON" : "OFF");
    LOG_D("设置套接字为信号驱动型IO:%s", get_fl(fd, O_ASYNC) ? "ON" : "OFF");
    LOG_D("设置套接属主:%ld", fcntl(fd, F_SETOWN, pthread_self()));
    LOG_D("获取套接属主:%ld", fcntl(fd, F_GETOWN));

    Close(fd);
}

/**
 * @brief ioctl
*/
static void print_socket_config_v3(void)
{
    int fd;

    fd = Socket(AF_INET, SOCK_STREAM, 0);
    // TODO:
    // 这里仅记录了参数, 实际使用还不行, 缺少参数且功能暂时未验证
    LOG_D("设置套接字为非阻塞式IO:%d", ioctl(fd, FIONBIO));
    LOG_D("设置套接字为信号驱动型IO:%d",ioctl(fd, FIOASYNC));
    LOG_D("设置套接属主:%ld", ioctl(fd, FIOSETOWN));
    LOG_D("获取套接属主:%ld", fcntl(fd, FIOGETOWN));
    LOG_D("获取套接字接收缓冲区中的字节数:%ld", ioctl(fd, FIONREAD));
    LOG_D("测试套接字是否处于带外标志:%ld", ioctl(fd, SIOCATMARK));
    LOG_D("获取接口列表:%d", ioctl(fd, SIOCGIFCONF));
    // LOG_D("接口操作:%d", ioctl(fd, SIOCGS))
    LOG_D("ARP高速缓存操作:%d", ioctl(fd, SIOCDARP)); //SIOCXARP
    LOG_D("路由表操作:%d", ioctl(fd, SIOCADDRT)); // SIOCXRT:SIOCADDRT, SIOCDELRT
    Close(fd);
}

