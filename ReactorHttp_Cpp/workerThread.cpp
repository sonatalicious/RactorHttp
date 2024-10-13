#include "workerThread.h"

#include <stdio.h>
#include <pthread.h>

WorkerThread::WorkerThread(int index)
{
	m_thread = nullptr;
	m_evLoop = nullptr;
	m_threadID = std::thread::id();
	m_name =  "SubThread - " + std::to_string(index);
}

WorkerThread::~WorkerThread()
{
	if (m_thread != nullptr)
	{
		delete m_thread;
	}
}

void WorkerThread::run()
{
	// 创建子线程
	m_thread = new std::thread(&WorkerThread::running,this);
	// 阻塞主线程，让当前函数不会直接结束
	std::unique_lock<std::mutex> locker(m_mutex); // 用 unique_lock 封装一下互斥锁，locker 构造时自动加锁
	while (nullptr == m_evLoop) // 要保证主线程取出 thread->evLoop 资源时，不会出现还没初始化，为 NULL 的情况
	{
		m_cond.wait(locker); // evLoop 初始化完成之前一直阻塞,期间释放 m_mutex 锁
	}
	// 这些加锁和条件变量的操作使得 workerThreadRun 函数在结束之前，evLoop 必定被成功初始化
	// 在多线程中，用到共享资源就需要加锁，线程间的某些操作有某种先后顺序则需要线程阻塞，即需要条件变量或信号
	// locker 在这里析构时会释放互斥锁，不用手动解锁
}

// 子线程的回调函数
void WorkerThread::running()
{
	m_mutex.lock(); // 对共享资源 thread->evLoop 加锁
	m_evLoop = new EventLoop(m_name);
	m_mutex.unlock(); // 解锁
	m_cond.notify_one(); // evLoop 已经成功初始化，现在解除主线程的阻塞
	m_evLoop->run();
}
