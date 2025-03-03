#include <stdio.h>
#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"

int TcpServer::acceptConnection(void* arg)
{
	struct TcpServer* server = static_cast<TcpServer*>(arg);
	// 和客户端建立连接
	int cfd = accept(server->m_lfd, nullptr, nullptr);
	// 从线程池中取出一个子线程的反应堆实例，去处理这个cfd
	EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();
	// 将cfd放到TcpConnection中处理
	new TcpConnection(cfd, evLoop);
	return 0;
}

void tcpServerRun(struct TcpServer* server)
{

}

TcpServer::TcpServer(unsigned short port, int threadNum)
{
	m_port = port;
	m_mainLoop = new EventLoop;
	m_threadNum = threadNum;
	m_threadPool = new ThreadPool(m_mainLoop, threadNum);
	setListen();
}

TcpServer::~TcpServer()
{
}

void TcpServer::setListen()
{
	// 1.创建监听的fd
	m_lfd = socket(AF_INET, SOCK_STREAM, 0); //参数1：选ipv4还是ipv6；参数2：tcp还是udp
	if (m_lfd == -1)
	{
		perror("socket");
		return;
	}
	// 2.设置端口复用
	// 当主动断开连接的一方需要一分钟才能释放绑定的端口，设置端口复用可以在这个时间内使用端口
	int opt = 1;// 开启端口复用
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1)
	{
		perror("setsockopt");
		return;
	}
	// 3.绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; // ip协议
	addr.sin_port = htons(m_port); // 绑定端口
	addr.sin_addr.s_addr = INADDR_ANY; // 绑定本地任意ip地址（网卡）
	ret = bind(m_lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1)
	{
		perror("bind");
		return;
	}
	// 4.设置监听
	ret = listen(m_lfd, 128); // 参数2：监听中一次性能和多少客户端建立连接，内核中最大为128
	if (ret == -1)
	{
		perror("listen");
		return;
	}
}

void TcpServer::run()
{
	Debug("服务器程序已经启动了...");
	// 启动线程池
	m_threadPool->run();
	// 添加检测的任务
	// 初始化一个channel实例
	Channel* channel = new Channel(m_lfd, FDEvent::ReadEvent, acceptConnection, nullptr, nullptr, this);
	m_mainLoop->addTask(channel, ElementType::ADD);
	// 启动反应堆模型
	m_mainLoop->run();
}
