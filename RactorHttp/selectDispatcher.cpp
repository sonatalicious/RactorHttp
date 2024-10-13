#include "dispatcher.h"

#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX 1024

struct SelectData
{
	fd_set readSet;
	fd_set writeSet;
};

static void* selectInit();
static int selectAdd(struct Channel* channel, struct EventLoop* evLoop);
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop);
static int selectModify(struct Channel* channel, struct EventLoop* evLoop);
static int selectDispatch(struct EventLoop* evLoop, int timeout);
static int selectClear(struct EventLoop* evLoop);

static void setFdSet(struct Channel* channel, struct SelectData* data);
static void clearFdSet(struct Channel* channel, struct SelectData* data);

struct Dispatcher SelectDispatcher = {
	selectInit,
	selectAdd,
	selectRemove,
	selectModify,
	selectDispatch,
	selectClear
};

static void* selectInit()
{
	struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
	FD_ZERO(&data->readSet); // 用宏函数设置集合为 0
	FD_ZERO(&data->writeSet);
	return data;
}

static void setFdSet(struct Channel* channel, struct SelectData* data)
{
	if (channel->events & ReadEvent) // 看看是否注册读事件
	{
		FD_SET(channel->fd, &data->readSet); // 把文件描述符设置到读集合中
	}
	if (channel->events & WriteEvent)// 看看是否注册写事件
	{
		FD_SET(channel->fd, &data->writeSet); // 把文件描述符设置到写集合中
	}
}
static void clearFdSet(struct Channel* channel, struct SelectData* data)
{
	if (channel->events & ReadEvent)
	{
		FD_CLR(channel->fd, &data->readSet); // 把文件描述符从读集合中删除(文件描述符对应的标志位置为0)
	}
	// 这里有逻辑错误，没有把 channel->fd 从读集合中去掉
	// 所以这里不检测 channel->events ，不管有没有都删掉这个fd的写事件
	// 或者直接用 FD_ZERO
	//if (channel->events & WriteEvent)
	//{
		FD_CLR(channel->fd, &data->writeSet);  // 把文件描述符从写集合中删除
	//}
}

static int selectAdd(struct Channel* channel, struct EventLoop* evLoop)
{
	// 先拿到 eventloop 中的 data 数据 data
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	if (channel->fd >= MAX)
	{
		return -1;
	}
	//if (channel->events & ReadEvent) // 看看是否注册读事件
	//{
	//	FD_SET(channel->fd, &data->readSet); // 把文件描述符设置到读集合中
	//}
	//if (channel->events & WriteEvent)// 看看是否注册写事件
	//{
	//	FD_SET(channel->fd, &data->writeSet); // 把文件描述符设置到写集合中
	//}
	setFdSet(channel, data);
	return 0;
}

static int selectRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	if (channel->fd >= MAX)
	{
		return -1;
	}
	//if (channel->events & ReadEvent)
	//{
	//	FD_CLR(channel->fd, &data->readSet); // 把文件描述符从读集合中删除(文件描述符对应的标志位置为0)
	//}
	//if (channel->events & WriteEvent)
	//{
	//	FD_CLR(channel->fd, &data->writeSet);  // 把文件描述符从写集合中删除
	//}
	clearFdSet(channel, data);
	// 通过 channel 注册的回调函数，释放相对应的 tcpConnection 资源
	channel->destroyCallback(channel->arg);
	return 0;
}

static int selectModify(struct Channel* channel, struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	clearFdSet(channel, data);
	setFdSet(channel, data);
	return 0;
}

#if 0
void print_fds_in_set(const fd_set* fds, int max_fd) {
	printf("File descriptors in set: ");
	for (int fd = 0; fd < max_fd; ++fd) {
		if (FD_ISSET(fd, fds)) {
			printf("%d ", fd);
		}
	}
	printf("\n");
}
#endif

static int selectDispatch(struct EventLoop* evLoop, int timeout)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	struct timeval val;
	val.tv_sec = timeout;
	val.tv_usec = 0;
	fd_set rdtmp = data->readSet;
	fd_set wrtmp = data->writeSet;
	int count = select(MAX, &rdtmp, &wrtmp, NULL, &val);
	if (-1 == count)
	{
#if 0
		print_fds_in_set(&rdtmp,MAX);
		print_fds_in_set(&wrtmp, MAX);
#endif
		Error("select -- errno:%d -- errnoMsg:%s",errno,strerror(errno));
		exit(0);
	}
	for (int i = 0; i < MAX; ++i) // 这个 i 相当于就是 fd
	{
		if (FD_ISSET(i, &rdtmp)) // 读事件被激活
		{
			eventActivate(evLoop, i, ReadEvent);
		}
		if (FD_ISSET(i, &wrtmp)) // 写事件被激活
		{
			eventActivate(evLoop, i, WriteEvent);
		}
	}
	return 0;
}
static int selectClear(struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	free(data);
	return 0;
}
