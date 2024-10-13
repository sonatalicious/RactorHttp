#include "eventLoop.h"
#include "log.h"
#include "selectDispatcher.h"
#include "pollDispatcher.h"
#include "epollDispatcher.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <string>

EventLoop::EventLoop() : EventLoop(std::string())
{ // 这是一个委托构造函数（C++11新特性），注意不要互相委托
}

EventLoop::EventLoop(const std::string threadName)
{
	m_isQuit = true; // 默认没有启动
	m_threadID = std::this_thread::get_id();
	m_threadName = threadName == std::string() ? "MainThread" : threadName;
	m_dispatcher = new SelectDispatcher(this);
	// channelMap
	m_channelMap.clear();
	// 初始化本地通信的内鬼 fd
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
	if (-1 == ret)
	{
		Error("socketpair");
		exit(0);
	}
#if 0
	// 添加内鬼 fd
	// 指定规则： evLoop->socketPair[0] 发送数据， evLoop->socketPair[1] 接收数据
	Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, \
		readLocalMessage, nullptr, nullptr, this);
#else
	// 绑定 bind -- C++11里的可调用对象绑定器
	// 将类的非静态成员函数绑定
	auto obj = std::bind(&EventLoop::readMessage, this);
	Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, \
		obj, nullptr, nullptr, this);
#endif
	// channel 添加到任务队列，这个 fd 其实是我们的内应，主线程向本地 fd 通信，来唤醒阻塞的主/子线程
	addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop() // 经过分析，不需要额外操作
{
}

int EventLoop::run()
{
	m_isQuit = false;
	// 比较线程ID是否正常
	if (m_threadID != std::this_thread::get_id())
	{
		Error("not this thread!");
		return -1;
	}
	// 循环进行事件处理
	while (!m_isQuit)
	{
		m_dispatcher->dispatch(); // 超时时长 2s
		processTaskQ(); // 处理一下任务队列
	}
	return 0;
}

int EventLoop::eventActive(int fd, int event)
{
	if (fd < 0)
	{
		return -1;
	}
	// 取出 channel
	Channel* channel = m_channelMap[fd];
	assert(channel->getSocket() == fd);
	if (event & (int)FDEvent::ReadEvent && channel->readCallback) // 如果是读事件，且其回调函数不为空
	{
		channel->readCallback(const_cast<void*>(channel->getArg())); // 调用相关读的回调函数
	}
	if (event & (int)FDEvent::WriteEvent && channel->writeCallback) // 如果是写事件，且其回调函数不为空
	{
		channel->writeCallback(const_cast<void*>(channel->getArg())); // 调用相关写的回调函数
	}
	return 0;
}

int EventLoop::addTask(Channel* channel, ElemType type)
{
	// 主线程和 eventloop 线程都会访问这块内存，所以要加锁，保护共享资源
	m_mutex.lock();
	// 创建新节点
	ChannelElement* node = new ChannelElement;
	node->channel = channel;
	node->type = type;
	// 将新节点放到队列中
	m_taskQ.push(node);
	m_mutex.unlock();
	// 处理节点
	/*
	细节：
	eventLoop 作为反应堆，在主线程和子线程都有
	1：当前线程为子线程时，可能是其他线程（主线程）添加了此链表节点
		1）修改 fd 事件是当前子线程发起的，当前子线程处理
		2）添加新 fd，添加任务节点的操作是由主线程发起的，外部对服务器建立了一个新连接
	2：不能让主线程处理任务队列，需要由当前的子线程去处理
	*/
	if (std::this_thread::get_id() == m_threadID)
	{
		// 当前为子线程（基于子线程的角度分析）
		processTaskQ();
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
		taskWakeup();
	}
	return 0;
}

int EventLoop::processTaskQ()
{
	// 取出头节点
	while (!m_taskQ.empty())
	{
		// 加锁，保护共享资源
		m_mutex.lock();
		ChannelElement* node = m_taskQ.front();
		m_taskQ.pop(); // 删除节点
		// 解锁，释放资源
		m_mutex.unlock();
		Channel* channel = node->channel;
		if (ElemType::ADD == node->type)
		{
			// 添加
			add(channel);
		}
		else if (ElemType::DELETE == node->type)
		{
			// 删除
			remove(channel);
		}
		else if (ElemType::MODIFY == node->type)
		{
			// 修改
			modify(channel);
		}
		delete node;
	}
	return 0;
}

int EventLoop::add(Channel* channel)
{
	int fd = channel->getSocket();
	// 找到 fd 对应的数组元素位置，并存储
	if (m_channelMap.end() == m_channelMap.find(fd)) // 看看 fd 在不在 m_channelMap 中
	{
		m_channelMap.insert(std::make_pair(fd, channel));
		m_dispatcher->setChannel(channel);
		int ret = m_dispatcher->add();
		return ret;
	}
	return -1;
}

int EventLoop::remove(Channel* channel)
{
	int fd = channel->getSocket();
	if (m_channelMap.end() == m_channelMap.find(fd)) // 看看 fd 在不在 m_channelMap 中
	{
		return -1;
	}
	m_dispatcher->setChannel(channel);
	int ret = m_dispatcher->remove();
	return ret;
}

int EventLoop::modify(Channel* channel)
{
	int fd = channel->getSocket();
	// 如果 fd 不在 channelMap 即检测集合中，或没找到 fd 对应的 channel
	if (m_channelMap.end() == m_channelMap.find(fd)) // 看看 fd 在不在 m_channelMap 中
	{
		return -1;
	}
	m_dispatcher->setChannel(channel);
	int ret = m_dispatcher->modify();
	return ret;
}

int EventLoop::freeChannel(Channel* channel)
{
	auto it = m_channelMap.find(channel->getSocket());
	if (m_channelMap.end() != it) // 看看 fd 在不在 m_channelMap 中
	{
		m_channelMap.erase(it);
		close(channel->getSocket());
		free(channel);
	}
	return 0;
}

// 本地写数据
void EventLoop::taskWakeup()
{
	const char* msg = "wake up! (placeholder) 唤醒子线程\n";
	write(m_socketPair[0], msg, strlen(msg));
}

// 本地读数据
// 由于是静态函数，属于类
// 不能调用类的某个实例的成员，所以将类的这个实例对象作为参数 arg 传入
int EventLoop::readLocalMessage(void* arg)
{
	EventLoop* evLoop = static_cast<EventLoop*>(arg);
	char buf[256];
	read(evLoop->m_socketPair[1], buf, sizeof(buf));
	// 不关心收到什么，解除阻塞就行
	return 0;
}

// 本地读数据
// 与上面的 readLocalMessage 相对应，成员函数属于对象实例，则不需要传参数了
int EventLoop::readMessage()
{
	char buf[256];
	read(m_socketPair[1], buf, sizeof(buf));
	// 不关心收到什么，解除阻塞就行
	return 0;
}
