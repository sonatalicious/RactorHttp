#include "threadPool.h"
#include "log.h"

#include <stdlib.h>
#include <assert.h>

struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count)
{
	struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
	pool->index = 0;
	pool->isStart = false;
	pool->mainLoop = mainLoop;
	pool->threadNum = count;
	pool->workerThreads = (struct WorkerThread*)malloc(sizeof(struct WorkerThread) * count);
	return pool;
}

void threadPoolRun(struct ThreadPool* pool)
{
	assert(pool && !pool->isStart); // �����̳߳��ѳ�ʼ����δ����
	if (pthread_self() != pool->mainLoop->threadID) // ���øú�����Ӧ�������߳�
	{
		Error("threadPoolRun not called by mainLoop");
		exit(0);
	}
	pool->isStart = true;
	if (pool->threadNum) // ����̳߳���Ӧ�������߳�
	{
		for (int i = 0; i < pool->threadNum; ++i)
		{
			workerThreadInit(&pool->workerThreads[i], i);
			workerThreadRun(&pool->workerThreads[i]);
		}
	}
}

struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
	assert(pool->isStart); // �̳߳�Ӧ���Ѿ�����
	if (pthread_self() != pool->mainLoop->threadID) // ���øú�����Ӧ�������߳�
	{
		Error("takeWorkerEventLoop not called by mainThread");
		exit(0);
	}
	// ���̳߳�����һ�����̣߳�Ȼ��ȡ������ķ�Ӧ��ʵ��
	struct EventLoop* evLoop = pool->mainLoop; // ��ָ��Ϊ���̵߳� eventLoop
	if (pool->threadNum > 0)
	{
		evLoop = pool->workerThreads[pool->index].evLoop;
		pool->index = ++pool->index % pool->threadNum; // �ı� pool->index ����֤���� 0~pool->threadNum ��Χ��
	}
	return evLoop;
}
