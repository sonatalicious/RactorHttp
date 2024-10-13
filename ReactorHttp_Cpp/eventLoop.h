#pragma once

#include "dispatcher.h"
#include "channel.h"

#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <string>

// 处理该节点中的 channel 的方式
enum class ElemType:char{ADD, DELETE, MODIFY};
// 定义任务队列的节点
struct ChannelElement
{
	ElemType type; // 如何处理该节点中的 channel
	Channel* channel;
};

class Dispatcher; // 先告诉编译器有这种类型

class EventLoop
{
public:
	EventLoop();
	EventLoop(const std::string threadName);
	~EventLoop();

	// 启动反应堆模型
	int run();
	// 处理被激活的 fd
	int eventActive(int fd, int event);
	// 添加任务到任务队列
	int addTask(Channel* channel, ElemType type);
	// 处理任务队列中的任务
	int processTaskQ();
	// 处理 dispatcher 中的节点(处理任务队列中的节点），增删改检测的 fd
	int add(Channel* channel);
	int remove(Channel* channel);
	int modify(Channel* channel);
	// 释放 channel
	int freeChannel(Channel* channel);
	// 返回线程 id
	inline std::thread::id getThreadID()
	{
		return m_threadID;
	}
	// channel.h 中 using handleFunc = int(*)(void* );
	// 注意 handleFunc 是函数指针，只能指向普通函数（类似 C 函数）或类静态函数
	// 因为类成员函数在初始化前不能存在，所以不能指向普通类成员函数
	static int readLocalMessage(void* arg);
	int readMessage();
private:
	void taskWakeup();

private:
	bool m_isQuit;
	// 该指针指向子类的实例 epoll，poll，Select
	Dispatcher* m_dispatcher;
	// 任务队列 taskQueue(主线程和此子线程公用，用来装如何进行）
	std::queue< ChannelElement* > m_taskQ;
	// channel map
	std::map<int, Channel*> m_channelMap;
	// 线程id，name，互斥锁 mutex(主要保护任务队列)
	std::thread::id m_threadID;
	std::string m_threadName;
	std::mutex m_mutex;
	// 存储本地通信的 fd，通过 socketpair 初始化
	int m_socketPair[2];
};
