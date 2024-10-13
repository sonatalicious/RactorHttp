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
	assert(!m_isStart); // �����̳߳��ѳ�ʼ����δ����
	if (std::this_thread::get_id() != m_mainLoop->getThreadID()) // ���øú�����Ӧ�������߳�
	{
		Error("threadPoolRun not called by mainLoop");
		exit(0);
	}
	m_isStart = true;
	if (m_threadNum > 0) // ����̳߳���Ӧ�������߳�
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
	assert(m_isStart); // �̳߳�Ӧ���Ѿ�����
	if (std::this_thread::get_id() != m_mainLoop->getThreadID()) // ���øú�����Ӧ�������߳�
	{
		Error("takeWorkerEventLoop not called by mainThread");
		exit(0);
	}
	// ���̳߳�����һ�����̣߳�Ȼ��ȡ������ķ�Ӧ��ʵ��
	EventLoop* evLoop = m_mainLoop; // ��ָ��Ϊ���̵߳� eventLoop
	if (m_threadNum > 0)
	{
		evLoop = m_workerThreads[m_index]->getEventLoop();
		m_index = ++m_index % m_threadNum; // �ı� m_index ����֤���� 0~m_threadNum ��Χ��
	}
	return evLoop;
}
