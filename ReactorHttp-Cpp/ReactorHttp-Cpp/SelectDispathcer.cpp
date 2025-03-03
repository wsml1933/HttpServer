#include "SelectDispathcer.h"

SelectDispatcher::SelectDispatcher(EventLoop* evLoop) : Dispatcher(evLoop)
{
	FD_ZERO(&m_readSet);
	FD_ZERO(&m_writeSet); //清零
	m_name = "Select";
}

SelectDispatcher::~SelectDispatcher()
{
}

int SelectDispatcher::add()
{
	if (m_channel->getSocket() >= m_maxSize)
	{
		return -1;
	}
	setFdSet();
	return 0;
}

int SelectDispatcher::remove()
{
	clearFdSet();
	// 通过channel释放对应的TcpConnection资源
	m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
	return 0;
}

int SelectDispatcher::modify()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
	{
		FD_SET(m_channel->getSocket(), &m_readSet);
		FD_CLR(m_channel->getSocket(), &m_writeSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
	{
		FD_SET(m_channel->getSocket(), &m_writeSet);
		FD_CLR(m_channel->getSocket(), &m_readSet);
	}
	return 0;
}

int SelectDispatcher::dispatch(int timeout)
{
	struct timeval val;
	val.tv_sec = timeout; // 秒
	val.tv_usec = 0; // 微秒
	fd_set rdtmp = m_readSet;
	fd_set wrtmp = m_writeSet;
	int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val); // +1是因为底层判断时是>,不会包含maxfd
	if (count == -1)
	{
		perror("select");
		exit(0);
	}
	for (int i = 0; i < m_maxSize; i++)
	{
		if (FD_ISSET(i, &rdtmp)) // 读集合的文件标识符被激活了
		{
			m_evLoop->eventActive(i, (int)FDEvent::ReadEvent);
		}

		if (FD_ISSET(i, &wrtmp))
		{
			m_evLoop->eventActive(i, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}
void SelectDispatcher::setFdSet()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
	{
		FD_SET(m_channel->getSocket(), &m_readSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
	{
		FD_SET(m_channel->getSocket(), &m_writeSet);
	}
}
void SelectDispatcher::clearFdSet()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
	{
		FD_CLR(m_channel->getSocket(), &m_readSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
	{
		FD_CLR(m_channel->getSocket(), &m_writeSet);
	}
}

