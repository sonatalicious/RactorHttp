#include "eventLoop.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

struct EventLoop* eventLoopInit()
{
	return eventLoopInitEx(NULL);
}

// 本地写数据
void taskWakeup(struct EventLoop* evLoop)
{
	const char* msg = "wake up! (placeholder) 唤醒子线程\n";
	write(evLoop->socketPair[0], msg, strlen(msg));
}
// 本地读数据
int readLocalMessage(void* arg)
{
	struct EventLoop* evLoop = (struct EventLoop*)arg;
	char buf[256];
	read(evLoop->socketPair[1], buf, sizeof(buf));
	// 不关心收到什么，解除阻塞就行
	return 0;
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
	struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
	evLoop->isQuit = false;
	evLoop->threadID = pthread_self();
	pthread_mutex_init(&evLoop->mutex, NULL);
	strcpy(evLoop->threadName, NULL == threadName ? "MainThread" : threadName);
	evLoop->dispatcher = &SelectDispatcher;
	evLoop->dispatcherData = evLoop->dispatcher->init();
	// 链表
	evLoop->head = evLoop->tail = NULL;
	// channelMap
	evLoop->channelMap = channelMapInit(128);
	// 初始化本地通信的内鬼 fd
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
	if (-1 == ret)
	{
		Error("socketpair");
		exit(0);
	}
	// 添加内鬼 fd
	// 指定规则： evLoop->socketPair[0] 发送数据， evLoop->socketPair[1] 接收数据
	struct Channel* channel = channelInit(evLoop->socketPair[1], ReadEvent, readLocalMessage, NULL,NULL, evLoop);
	// channel 添加到任务队列，这个 fd 其实是我们的内应，主线程向本地 fd 通信，来唤醒阻塞的主/子线程
	eventLoopAddTask(evLoop, channel, ADD);
	return evLoop;
}

int eventLoopRun(struct EventLoop* evLoop)
{
	assert(evLoop != NULL);
	// 取出事件分发和检测模型
	struct Dispatcher* dispatcher = evLoop->dispatcher;
	// 比较线程ID是否正常
	if (evLoop->threadID != pthread_self())
	{
		Error("not this thread!");
		return - 1;
	}
	// 循环进行事件处理
	while (!evLoop->isQuit)
	{
		dispatcher->dispatch(evLoop, 2); // 超时时长 2s
		eventLoopProcessTask(evLoop); // 处理一下任务队列
	}
	return 0;
}

int eventActivate(struct EventLoop* evLoop, int fd, int event)
{
	if (NULL == evLoop || fd < 0)
	{
		return -1;
	}
	// 取出 channel
	struct Channel* channel = evLoop->channelMap->list[fd];
	assert(channel->fd == fd);
	if (event & ReadEvent && channel->readCallback) // 如果是读事件，且其回调函数不为空
	{
		channel->readCallback(channel->arg); // 调用相关读的回调函数
	}
	if (event & WriteEvent && channel->writeCallback) // 如果是写事件，且其回调函数不为空
	{
		channel->writeCallback(channel->arg); // 调用相关写的回调函数
	}
	return 0;
}

int eventLoopAddTask(struct EventLoop* evLoop,struct Channel* channel, int type)
{
	// 主线程和 eventloop 线程都会访问这块内存，所以要加锁，保护共享资源
	pthread_mutex_lock(&evLoop->mutex);
	// 创建新节点
	struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
	node->channel = channel;
	node-> type = type;
	node->next = NULL;
	// 将新节点放到链表中
	if (NULL == evLoop->head)
	{
		evLoop->head = evLoop->tail = node;
	}
	else
	{
		evLoop->tail->next = node; // 添加
		evLoop->tail = node; // tail 后移
	}
	pthread_mutex_unlock(&evLoop->mutex); // 解锁
	// 处理节点
	/*
	细节：
	eventLoop 作为反应堆，在主线程和子线程都有
	1：当前线程为子线程时，可能是其他线程（主线程）添加了此链表节点
		1）修改 fd 事件是当前子线程发起的，当前子线程处理
		2）添加新 fd，添加任务节点的操作是由主线程发起的，外部对服务器建立了一个新连接
	2：不能让主线程处理任务队列，需要由当前的子线程去处理
	*/
	if (evLoop->threadID == pthread_self())
	{
		// 当前为子线程（基于子线程的角度分析）
		eventLoopProcessTask(evLoop);
	}
	else
	{
		// 当前为主线程 -- 告诉子线程去处理任务队列中的任务
		// 此时有两种情况：
		// 1.子线程在工作 
		// 2.子线程被阻塞了：select，poll，epoll
		//（没有事件时底层的这三个IO多路复用模型会阻塞，时长最长设置为 2s）

		// 向内应 socket：evLoop->socketPair[1] 写数据
		// evLoop->socketPair[1] 会收数据，以此唤醒阻塞的子线程
		taskWakeup(evLoop);
	}
	return 0;
}


int eventLoopProcessTask(struct EventLoop* evLoop)
{
	// 加锁，保护共享资源
	pthread_mutex_lock(&evLoop->mutex);
	// 取出头节点
	struct ChannelElement* head = evLoop->head;
	while (NULL != head)
	{
		struct Channel* channel = head->channel;
		if (ADD == head->type)
		{
			// 添加
			eventLoopAdd(evLoop, channel);
		}
		else if (DELETE == head->type)
		{
			// 删除
			eventLoopRemove(evLoop, channel);
		}		
		else if (MODIFY == head->type)
		{
			// 修改
			eventLoopModify(evLoop, channel);
		}
		struct ChannelElement* tmp = head;
		head = head->next;
		free(tmp);
	}
	// 任务都被处理完了，重新初始化一下
	evLoop->head = evLoop->tail = NULL;
	// 解锁，释放资源
	pthread_mutex_unlock(&evLoop->mutex);
	return 0;
}

int eventLoopAdd(EventLoop* evLoop, Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size)
	{
		// 当 channelmap 没有空间存储此 fd 对应的 channel 时 ==> 扩容
		if (!makeMapRoom(channelMap, fd, sizeof(struct Channel*)))
		{
			// 扩容失败
			return -1;
		}
	}
	// 找到 fd 对应的数组元素位置，并存储
	if (NULL == channelMap->list[fd])
	{
		channelMap->list[fd] = channel;
		evLoop->dispatcher->add(channel, evLoop);
	}
	return 0;
}
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size) // 如果 fd 不在 channelMap 即检测集合中
	{
			return -1;
	}
	int ret = evLoop->dispatcher->remove(channel, evLoop);
	return ret;
}
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	// 如果 fd 不在 channelMap 即检测集合中，或没找到 fd 对应的 channel
	if (fd >= channelMap->size || NULL == channelMap->list[fd]) 
	{
		return -1;
	}
	int ret = evLoop->dispatcher->modify(channel, evLoop);
	return ret;
}
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel)
{
	// 删除 channel 和 fd 的对应关系
	evLoop->channelMap->list[channel->fd] = NULL;
	// 关闭 fd
	close(channel->fd);
	// 释放 channel
	free(channel);
	return 0;
}