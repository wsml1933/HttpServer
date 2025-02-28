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
		channel->events |= WriteEvent; // 100进位或，将其置为1
	}
	else
	{
		channel->events = channel->events & ~WriteEvent; //先取反后三位变成011，再&将其置为零
	}
}

bool isWriteEventEnable(struct Channel* channel)
{
	return channel->events & WriteEvent; // 如果大于0则需要，等于零则不需要
}
