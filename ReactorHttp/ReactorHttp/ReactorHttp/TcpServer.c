#include <stdio.h>
#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"

struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
	struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
	tcp->listener = listenerInit(port);
	tcp->mainLoop = eventLoopInit();
	tcp->threadNum = threadNum;
	tcp->threadPool = threadPoolInit(tcp->mainLoop, threadNum);
	return tcp;
}

struct Listener* listenerInit(unsigned short port)
{
	struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
	// 1.创建监听的fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0); //参数1：选ipv4还是ipv6；参数2：tcp还是udp
	if (lfd == -1)
	{
		perror("socket");
		return NULL;
	}
	// 2.设置端口复用
	// 当主动断开连接的一方需要一分钟才能释放绑定的端口，设置端口复用可以在这个时间内使用端口
	int opt = 1;// 开启端口复用
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1)
	{
		perror("setsockopt");
		return NULL;
	}
	// 3.绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; // ip协议
	addr.sin_port = htons(port); // 绑定端口
	addr.sin_addr.s_addr = INADDR_ANY; // 绑定本地任意ip地址（网卡）
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1)
	{
		perror("bind");
		return NULL;
	}
	// 4.设置监听
	ret = listen(lfd, 128); // 参数2：监听中一次性能和多少客户端建立连接，内核中最大为128
	if (ret == -1)
	{
		perror("listen");
		return NULL;
	}
	// 返回fd
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}
int acceptConnection(void* arg)
{
	struct TcpServer* server = (struct TcpServer*)arg;
	// 和客户端建立连接
	int cfd = accept(server->listener->lfd, NULL, NULL);
	// 从线程池中取出一个子线程的反应堆实例，去处理这个cfd
	struct EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
	// 将cfd放到TcpConnection中处理
	tcpConnectionInit(cfd, evLoop);
	return 0;
}

void tcpServerRun(struct TcpServer* server)
{
	Debug("服务器程序已经启动了...");
	// 启动线程池
	threadPoolRun(server->threadPool);
	// 添加检测的任务
	// 初始化一个channel实例
	struct Channel* channel = channelInit(server->listener->lfd,
		ReadEvent, acceptConnection, NULL, NULL, server);
	eventLoopAddTask(server->mainLoop, channel, ADD);
	// 启动反应堆模型
	eventLoopRun(server->mainLoop);
}
