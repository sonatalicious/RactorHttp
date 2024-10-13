#pragma once

#include <pthread.h>

#include "dispatcher.h"
#include "channelMap.h"

extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;

// 处理该节点中的 channel 的方式
enum ElemType{ADD, DELETE, MODIFY};
// 定义任务队列的节点
struct ChannelElement
{
	int type; // 如何处理该节点中的 channel
	struct Channel* channel;
	struct ChannelElement* next;
};

struct Dispatcher; // 先告诉编译器有这种类型

struct EventLoop
{
	bool isQuit;
	struct Dispatcher* dispatcher;
	void* dispatcherData;
	// 任务队列 taskQueue(主线程和此子线程公用，用来装如何进行）
	struct ChannelElement* head;
	struct ChannelElement* tail;
	// channel map
	struct ChannelMap* channelMap;
	// 线程id，name，互斥锁 mutex(主要保护任务队列)
	pthread_t threadID;
	char threadName[32];
	pthread_mutex_t mutex;
	// 存储本地通信的 fd，通过 socketpair 初始化
	int socketPair[2];
};

// 初始化
struct EventLoop* eventLoopInit(); // 初始化主线程
struct EventLoop* eventLoopInitEx(const char* threadName); // 初始化子线程
// 启动反应堆模型
int eventLoopRun(struct EventLoop* evLoop);
// 处理被激活的 fd
int eventActivate(struct EventLoop* evLoop, int fd, int event);
// 添加任务到任务队列
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type);
// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop* evLoop);
// 处理 dispatcher 中的节点(处理任务队列中的节点），增删改检测的 fd
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
// 释放 channel
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel);
