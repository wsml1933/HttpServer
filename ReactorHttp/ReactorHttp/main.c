#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "TcpServer.h"

int main(int argc, char* argv[])
{
#if 0
    // argv要传三个参数，可执行程序名字、端口、资源目录
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    chdir(argv[2]);
#else
    unsigned short port = 10000;
    chdir("/root/sharedata/learn/httpdir");
#endif
    // 启动服务器
    struct TcpServer* server = tcpServerInit(port, 4);
    tcpServerRun(server);

    return 0;
}