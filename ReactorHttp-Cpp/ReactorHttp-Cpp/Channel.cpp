#include "Channel.h"
#include <stdlib.h>

Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg)
{
	m_arg = arg;
	m_events = (int)events;
	m_fd = fd;
	readCallback = readFunc;
	writeCallback = writeFunc;
	destroyCallback = destroyFunc;
}

void Channel::writeEventEnable(bool flag)
{
	if (flag)
	{
		//m_events |= (int)FDEvent::WriteEvent; // 100��λ�򣬽�����Ϊ1
		m_events |= static_cast<int>(FDEvent::WriteEvent); // 100��λ�򣬽�����Ϊ1
	}
	else
	{
		m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent); //��ȡ������λ���011����&������Ϊ��
	}
}

bool Channel::isWriteEventEnable()
{
	return m_events & static_cast<int>(FDEvent::WriteEvent); // �������0����Ҫ������������Ҫ
}
