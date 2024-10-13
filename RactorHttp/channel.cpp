#include "channel.h"

#include <stdlib.h>
#include <stdio.h>

struct Channel* channelInit(int fd, int events, handleFunc readFunc,
	handleFunc writeFunc, handleFunc destroyFunc, void* arg)
{
	struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel));
	channel->fd = fd;
	channel->events = events;
	channel->readCallback = readFunc;
	channel->writeCallback = writeFunc;
	channel->destroyCallback = destroyFunc;
	channel->arg = arg;
	return channel;
}

void writeEventEnable(struct Channel* channel, bool flag)
{
	if (flag)
	{
		channel->events |= WriteEvent;
	}
	else
	{
		channel->events = channel->events & ~WriteEvent;
	}
}

bool isWriteEventEnable(struct Channel* channel)
{
	return channel->events & WriteEvent; // 若成立则得到大于0的数(true)，否则为0(false)
}
