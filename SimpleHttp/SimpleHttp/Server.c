#include "Server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <fcntl.h>

int initListenFd(unsigned short port)
{
	// 1.创建监听的fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0); //参数1：选ipv4还是ipv6；参数2：tcp还是udp
	if (lfd == -1)
	{
		perror("socket");
		return -1;
	}
	// 2.设置端口复用
	// 当主动断开连接的一方需要一分钟才能释放绑定的端口，设置端口复用可以在这个时间内使用端口
	int opt = 1;// 开启端口复用
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt); 
	if (ret == -1)
	{
		perror("setsockopt");
		return -1;
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
		return -1;
	}
	// 4.设置监听
	ret = listen(lfd, 128); // 参数2：监听中一次性能和多少客户端建立连接，内核中最大为128
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	// 返回fd
	return lfd;
}

int epollRun(int lfd)
{
	// 1.创建epoll实例
	int epfd = epoll_create(1); // 参数被弃用了，大于零即可，若为零则创建失败
	if (epfd == -1)
	{
		perror("epoll_create");
		return -1;
	}
	// 2.lfd 上树（红黑树）
	struct epoll_event ev;
	ev.data.fd = lfd; // 记录当前要操作的fd 
	ev.events = EPOLLIN; // 委托epoll检测的事件 EPOLLIN读事件
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev); // 添加
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	// 3.检测
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event); // evs数组的大小

	while (1)
	{
		int num = epoll_wait(epfd, evs, size, -1); // 设置为-1，没有事件则一直堵塞，有事件触发了则返回事件触发的个数
		for (int i = 0; i < num; i++)
		{
			int fd = evs[i].data.fd;
			if (fd == lfd) // 如果是我要监听的fd
			{
				// 建立新连接 accept，此时不会堵塞，因为已经找到要建立的新连接的fd，accept不需要阻塞
				acceptClient(lfd, epfd); // 建立新连接，并把新得到的cfd添加到epoll树上
			}
			else // 如果不是我要监听的fd，则说明是进行通信的fd，发送接收数据
			{
				// 主要是接受对端的数据
			}
		}
	}

	return 0;
}

int acceptClient(int lfd, int epfd)
{
	// 1. 建立连接
	int cfd = accpet(lfd, NULL, NULL);
	if (cfd == -1)
	{
		perror("accept");
		return -1;
	}
	// 2. 设置非阻塞
	// 边缘和水平，其中边缘非阻塞效率最高，fd默认阻塞，需要设置为非阻塞
	int flag = fcntl(cfd, F_GETFL); // 获取fd的属性
	flag = O_NONBLOCK; // 设置非阻塞属性
	fcntl(cfd, F_SETFL, flag); // 将修改后的属性设置回cfd

	// 3. cdf添加到epoll中
	struct epoll_event ev;
	ev.data.fd = cfd; 
	ev.events = EPOLLIN | EPOLLET; // 默认为水平设置边缘触发
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	return 0;
}
