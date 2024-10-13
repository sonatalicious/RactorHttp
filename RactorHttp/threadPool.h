#pragma once

#include "eventLoop.h"
#include "workerThread.h"

// �����̳߳�
struct ThreadPool
{
	// ���̵߳ķ�Ӧ��ģ��
	struct EventLoop* mainLoop;
	bool isStart;
	int threadNum;
	struct WorkerThread* workerThreads;
	int index;
};

// ��ʼ���̳߳�
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count);
// �����̳߳�
void threadPoolRun(struct ThreadPool* pool);
// ȡ���̳߳���ĳ�����̵߳ķ�Ӧ��ʵ��
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);
