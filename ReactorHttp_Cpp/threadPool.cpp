#include "threadPool.h"
#include "log.h"

#include <stdlib.h>
#include <assert.h>

ThreadPool::ThreadPool(EventLoop* mainLoop, int count)
{
	m_index = 0;
	m_isStart = false;
	m_mainLoop = mainLoop;
	m_threadNum = count;
	m_workerThreads.clear();
}

ThreadPool::~ThreadPool()
{
	for (auto item : m_workerThreads)
	{
		delete item;
	}
}

void ThreadPool::run()
{
	assert(!m_isStart); // 断言线程池已初始化并未启用
	if (std::this_thread::get_id() != m_mainLoop->getThreadID()) // 调用该函数的应该是主线程
	{
		Error("threadPoolRun not called by mainLoop");
		exit(0);
	}
	m_isStart = true;
	if (m_threadNum > 0) // 如果线程池中应该有子线程
	{
		for (int i = 0; i < m_threadNum; ++i)
		{
			WorkerThread* subThread = new WorkerThread(i);
			subThread->run();
			m_workerThreads.push_back(subThread);
		}
	}
}

EventLoop* ThreadPool::takeWorkerEventLoop()
{
	assert(m_isStart); // 线程池应该已经启动
	if (std::this_thread::get_id() != m_mainLoop->getThreadID()) // 调用该函数的应该是主线程
	{
		Error("takeWorkerEventLoop not called by mainThread");
		exit(0);
	}
	// 从线程池中找一个子线程，然后取出里面的反应堆实例
	EventLoop* evLoop = m_mainLoop; // 先指定为主线程的 eventLoop
	if (m_threadNum > 0)
	{
		evLoop = m_workerThreads[m_index]->getEventLoop();
		m_index = ++m_index % m_threadNum; // 改变 m_index 并保证其在 0~m_threadNum 范围内
	}
	return evLoop;
}
