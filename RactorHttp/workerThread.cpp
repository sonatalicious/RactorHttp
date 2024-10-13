#include "workerThread.h"

#include <stdio.h>
#include <pthread.h>

int workerThreadInit(struct WorkerThread* thread, int index)
{
	thread->evLoop = NULL;
	thread->threadID = 0;
	sprintf(thread->name, "SubThread - %d", index);
	pthread_mutex_init(&thread->mutex, NULL);
	pthread_cond_init(&thread->cond, NULL);
	return 0;
}
// 子线程的回调函数
void* subThreadRunning(void* arg)
{
	struct WorkerThread* thread = (struct WorkerThread*)arg;
	pthread_mutex_lock(&thread->mutex); // 对共享资源 thread->evLoop 加锁
	thread->evLoop = eventLoopInitEx(thread->name);
	pthread_mutex_unlock(&thread->mutex); // 解锁
	pthread_cond_signal(&thread->cond); // evLoop 已经成功初始化，现在解除主线程的阻塞
	eventLoopRun(thread->evLoop);
	return NULL;
}

void workerThreadRun(struct WorkerThread* thread) // 这是主线程调用的函数
{
	// 创建子线程
	pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
	// 阻塞主线程，让当前函数不会直接结束
	pthread_mutex_lock(&thread->mutex); // thread->evLoop 是共享资源，所以加锁
	while (NULL == thread->evLoop) // 要保证主线程取出 thread->evLoop 资源时，不会出现还没初始化，为 NULL 的情况
	{
		pthread_cond_wait(&thread->cond, &thread->mutex); // evLoop 初始化完成之前一直阻塞
	}
	pthread_mutex_unlock(&thread->mutex);
	// 这些加锁和条件变量的操作使得 workerThreadRun 函数在结束之前，evLoop 必定被成功初始化
	// 在多线程中，用到共享资源就需要加锁，线程间的某些操作有某种先后顺序则需要线程阻塞，即需要条件变量或信号
}
