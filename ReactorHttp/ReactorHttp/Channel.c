#include "Channel.h"
#include <stdlib.h>

struct Channel* channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg)
{
	struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel));
	channel->arg = arg;
	channel->events = events;
	channel->fd = fd;
	channel->readCallback = readFunc;
	channel->writeCallback = writeFunc;
	channel->destroyCallback = destroyFunc;
	return channel;
}

void writeEventEnable(struct Channel* channel, bool flag)
{
	if (flag)
	{
		channel->events |= WriteEvent; // 100��λ�򣬽�����Ϊ1
	}
	else
	{
		channel->events = channel->events & ~WriteEvent; //��ȡ������λ���011����&������Ϊ��
	}
}

bool isWriteEventEnable(struct Channel* channel)
{
	return channel->events & WriteEvent; // �������0����Ҫ������������Ҫ
}
