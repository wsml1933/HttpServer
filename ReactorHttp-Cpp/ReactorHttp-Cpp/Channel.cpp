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
		//m_events |= (int)FDEvent::WriteEvent; // 100进位或，将其置为1
		m_events |= static_cast<int>(FDEvent::WriteEvent); // 100进位或，将其置为1
	}
	else
	{
		m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent); //先取反后三位变成011，再&将其置为零
	}
}

bool Channel::isWriteEventEnable()
{
	return m_events & static_cast<int>(FDEvent::WriteEvent); // 如果大于0则需要，等于零则不需要
}
