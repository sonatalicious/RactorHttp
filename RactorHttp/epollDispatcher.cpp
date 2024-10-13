#include "dispatcher.h"
#include "log.h"

#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define MAX 520

struct EpollData
{
	int epfd;
	struct epoll_event* events;
};

static void* epollInit();
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int epollModify(struct Channel* channel, struct EventLoop* evLoop);
static int epollDispatch(struct EventLoop* evLoop, int timeout);
static int epollClear(struct EventLoop* evLoop);

//用来精简函数（相似代码提取出来）
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op);

struct Dispatcher EpollDispatcher = {
	epollInit,
	epollAdd,
	epollRemove,
	epollModify,
	epollDispatch,
	epollClear
};

static void* epollInit()
{
	struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
	data->epfd = epoll_create(10);
	if (-1 == data->epfd)
	{
		Error("epoll_create");
		exit(0);
	}
	data->events = (struct epoll_event*)calloc(MAX, sizeof(struct epoll_event));
	return data;
}

static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int events = 0;
	if (channel->events & ReadEvent) // 看看是否注册读事件
	{
		events |= EPOLLIN;
	}
	if (channel->events & WriteEvent)// 看看是否注册写事件
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
	return ret;
}

static int epollAdd(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
	if (-1 == ret)
	{
		Error("EPOLL_CTL add");
		exit(0);
	}
	return ret;
}
//static int epollAdd(struct Channel* channel, struct EventLoop* evLoop)
//{
//	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
//	struct epoll_event ev;
//	ev.data.fd = channel->fd;
//	int events = 0;
//	if (channel->events & ReadEvent) // 看看是否注册读事件
//	{
//		events |= EPOLLIN;
//	}
//	if (channel->events & WriteEvent)// 看看是否注册写事件
//	{
//		events |= EPOLLOUT;
//	}
//	ev.events = events;
//	int ret = epoll_ctl(data->epfd, EPOLL_CTL_ADD, channel->fd, &ev);
//	return ret;
//}

static int epollRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
	if (-1 == ret)
	{
		Error("EPOLL_CTL delete");
		exit(0);
	}
	// 通过 channel 注册的回调函数，释放相对应的 tcpConnection 资源
	channel->destroyCallback(channel->arg);
	return ret;
}
//static int epollRemove(struct Channel* channel, struct EventLoop* evLoop)
//{
//	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
//	struct epoll_event ev;
//	ev.data.fd = channel->fd;
//	int events = 0;
//	if (channel->events & ReadEvent)
//	{
//		events |= EPOLLIN;
//	}
//	if (channel->events & WriteEvent)
//	{
//		events |= EPOLLOUT;
//	}
//	ev.events = events;
//	// 其实这里的 &ev 可以设置为空，上面的许多操作代码就可以删掉了
//	int ret = epoll_ctl(data->epfd, EPOLL_CTL_DEL, channel->fd, &ev); 
//	return ret;
//}

static int epollModify(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
	if (-1 == ret)
	{
		Error("EPOLL_CTL modify");
		exit(0);
	}
	return ret;
}
//static int epollModify(struct Channel* channel, struct EventLoop* evLoop)
//{
//	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
//	struct epoll_event ev;
//	ev.data.fd = channel->fd;
//	int events = 0;
//	if (channel->events & ReadEvent)
//	{
//		events |= EPOLLIN;
//	}
//	if (channel->events & WriteEvent)
//	{
//		events |= EPOLLOUT;
//	}
//	ev.events = events;
//	int ret = epoll_ctl(data->epfd, EPOLL_CTL_MOD, channel->fd, &ev);
//	return ret;
//}

static int epollDispatch(struct EventLoop* evLoop, int timeout)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	int count = epoll_wait(data->epfd, data->events, MAX, timeout * 1000);
	for (int i = 0; i < count; ++i)
	{
		int events = data->events[i].events;
		int fd = data->events[i].data.fd;
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			// 对方断开了连接，删除 fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN) // 读事件被触发
		{
			eventActivate(evLoop, fd, ReadEvent);

		}
		if (events & EPOLLOUT) // 写事件被触发
		{
			eventActivate(evLoop, fd, WriteEvent);
		}
	}
	return 0;
}
static int epollClear(struct EventLoop* evLoop)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	free(data->events);
	close(data->epfd);
	free(data);
	return 0;
}
