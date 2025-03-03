#include "EpollDispathcer.h"
#include "Dispatcher.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

EpollDispatcher::EpollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop)
{
	m_epfd = epoll_create(1); // 大于0即可参数已废弃
	if (m_epfd == -1)
	{
		perror("epoll_create");
		exit(0);
	}
	m_events = new struct epoll_event[m_maxNode];
	m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher()
{
	close(m_epfd);
	delete[]m_events;
}

int EpollDispatcher::add()
{
	int ret = epollCtl(EPOLL_CTL_ADD);
	if (ret == -1)
	{
		perror("epoll_ctl add");
		exit(0);
	}
	return ret;
	return 0;
}

int EpollDispatcher::remove()
{
	int ret = epollCtl(EPOLL_CTL_DEL);
	if (ret == -1)
	{
		perror("epoll_ctl delete");
		exit(0);
	}
	// 通过channel释放对应的TcpConnection资源
	m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));// const_cast去掉后面参数里面的只读属性
	return ret;
	return 0;
}

int EpollDispatcher::modify()
{
	int ret = epollCtl(EPOLL_CTL_MOD);
	if (ret == -1)
	{
		perror("epoll_ctl modify");
		exit(0);
	}
	return ret;
	return 0;
}

int EpollDispatcher::dispatch(int timeout)
{
	int count = epoll_wait(m_epfd, m_events, m_maxNode, timeout * 1000); // timeout * 1000 将秒转为毫秒
	for (int i = 0; i < count; i++)
	{
		int events = m_events[i].events;
		int fd = m_events[i].data.fd;
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			// 对方断开连接，删除fd
			// epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN)
		{
			m_evLoop->eventActive(fd, (int)FDEvent::ReadEvent);
		}
		if (events & EPOLLOUT)
		{
			m_evLoop->eventActive(fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}

int EpollDispatcher::epollCtl(int op)
{
	struct epoll_event ev;
	ev.data.fd = m_channel->getSocket();
	int events = 0;
	if (m_channel->getEvent() & static_cast<int>(FDEvent::ReadEvent))
	{
		events |= EPOLLIN;
	}
	if (m_channel->getEvent() & static_cast<int>(FDEvent::WriteEvent))
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
	return ret;
}