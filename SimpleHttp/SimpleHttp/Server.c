#include "Server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <fcntl.h>

int initListenFd(unsigned short port)
{
	// 1.����������fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0); //����1��ѡipv4����ipv6������2��tcp����udp
	if (lfd == -1)
	{
		perror("socket");
		return -1;
	}
	// 2.���ö˿ڸ���
	// �������Ͽ����ӵ�һ����Ҫһ���Ӳ����ͷŰ󶨵Ķ˿ڣ����ö˿ڸ��ÿ��������ʱ����ʹ�ö˿�
	int opt = 1;// �����˿ڸ���
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt); 
	if (ret == -1)
	{
		perror("setsockopt");
		return -1;
	}
	// 3.��
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; // ipЭ��
	addr.sin_port = htons(port); // �󶨶˿�
	addr.sin_addr.s_addr = INADDR_ANY; // �󶨱�������ip��ַ��������
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1)
	{
		perror("bind");
		return -1;
	}
	// 4.���ü���
	ret = listen(lfd, 128); // ����2��������һ�����ܺͶ��ٿͻ��˽������ӣ��ں������Ϊ128
	if (ret == -1)
	{
		perror("listen");
		return -1;
	}
	// ����fd
	return lfd;
}

int epollRun(int lfd)
{
	// 1.����epollʵ��
	int epfd = epoll_create(1); // �����������ˣ������㼴�ɣ���Ϊ���򴴽�ʧ��
	if (epfd == -1)
	{
		perror("epoll_create");
		return -1;
	}
	// 2.lfd �������������
	struct epoll_event ev;
	ev.data.fd = lfd; // ��¼��ǰҪ������fd 
	ev.events = EPOLLIN; // ί��epoll�����¼� EPOLLIN���¼�
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev); // ���
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	// 3.���
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event); // evs����Ĵ�С

	while (1)
	{
		int num = epoll_wait(epfd, evs, size, -1); // ����Ϊ-1��û���¼���һֱ���������¼��������򷵻��¼������ĸ���
		for (int i = 0; i < num; i++)
		{
			int fd = evs[i].data.fd;
			if (fd == lfd) // �������Ҫ������fd
			{
				// ���������� accept����ʱ�����������Ϊ�Ѿ��ҵ�Ҫ�����������ӵ�fd��accept����Ҫ����
				acceptClient(lfd, epfd); // ���������ӣ������µõ���cfd��ӵ�epoll����
			}
			else // ���������Ҫ������fd����˵���ǽ���ͨ�ŵ�fd�����ͽ�������
			{
				// ��Ҫ�ǽ��ܶԶ˵�����
			}
		}
	}

	return 0;
}

int acceptClient(int lfd, int epfd)
{
	// 1. ��������
	int cfd = accpet(lfd, NULL, NULL);
	if (cfd == -1)
	{
		perror("accept");
		return -1;
	}
	// 2. ���÷�����
	// ��Ե��ˮƽ�����б�Ե������Ч����ߣ�fdĬ����������Ҫ����Ϊ������
	int flag = fcntl(cfd, F_GETFL); // ��ȡfd������
	flag = O_NONBLOCK; // ���÷���������
	fcntl(cfd, F_SETFL, flag); // ���޸ĺ���������û�cfd

	// 3. cdf��ӵ�epoll��
	struct epoll_event ev;
	ev.data.fd = cfd; 
	ev.events = EPOLLIN | EPOLLET; // Ĭ��Ϊˮƽ���ñ�Ե����
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl");
		return -1;
	}
	return 0;
}
