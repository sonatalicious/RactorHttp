#pragma once

#include <vector>

#include "eventLoop.h"
#include "workerThread.h"

// �����̳߳�
class ThreadPool
{
public:
	ThreadPool(EventLoop* mainLoop, int count);
	~ThreadPool();
	// �����̳߳�
	void run();
	// ȡ���̳߳���ĳ�����̵߳ķ�Ӧ��ʵ��
	EventLoop* takeWorkerEventLoop();

private:
	// ���̵߳ķ�Ӧ��ģ��
	EventLoop* m_mainLoop;
	bool m_isStart;
	int m_threadNum;
	std::vector<WorkerThread*> m_workerThreads;
	int m_index;
};

