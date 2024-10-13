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
	assert(pool && !pool->isStart); // 断言线程池已初始化并未启用
	if (pthread_self() != pool->mainLoop->threadID) // 调用该函数的应该是主线程
	{
		Error("threadPoolRun not called by mainLoop");
		exit(0);
	}
	pool->isStart = true;
	if (pool->threadNum) // 如果线程池中应该有子线程
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
	assert(pool->isStart); // 线程池应该已经启动
	if (pthread_self() != pool->mainLoop->threadID) // 调用该函数的应该是主线程
	{
		Error("takeWorkerEventLoop not called by mainThread");
		exit(0);
	}
	// 从线程池中找一个子线程，然后取出里面的反应堆实例
	struct EventLoop* evLoop = pool->mainLoop; // 先指定为主线程的 eventLoop
	if (pool->threadNum > 0)
	{
		evLoop = pool->workerThreads[pool->index].evLoop;
		pool->index = ++pool->index % pool->threadNum; // 改变 pool->index 并保证其在 0~pool->threadNum 范围内
	}
	return evLoop;
}
