#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "Server.h"
int main(int argc, char* argv[])
{
    // argv要传三个参数，可执行程序名字、端口、资源目录
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    chdir(argv[2]);
    // 初始化用于监听的套接字
    int lfd = initListenFd(port); //short两个字节，16位，2的16次方，0-65535，尽量不要找5000以下的（其他程序占用）
    // 启动服务器程序
    epollRun(lfd);
    return 0;
}