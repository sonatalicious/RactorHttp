#pragma once

#include "eventLoop.h"

#include <thread>
#include <mutex>
#include <condition_variable>

// �������̶߳�Ӧ�Ľṹ��
class WorkerThread
{
public:
	WorkerThread(int index);
	~WorkerThread();
	// �����߳�
	void run();
	inline EventLoop* getEventLoop()
	{
		return m_evLoop;
	}
private:
	void running();
private:
	std::thread* m_thread; // �߳�ָ�룬���������̵߳�ʵ��
	std::thread::id m_threadID; // ID
	std::string m_name;
	std::mutex m_mutex; // ������
	std::condition_variable m_cond; // ��������
	EventLoop* m_evLoop; // ��Ӧ��ģ��
};
