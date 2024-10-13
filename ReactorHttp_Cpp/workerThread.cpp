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
	// �������߳�
	m_thread = new std::thread(&WorkerThread::running,this);
	// �������̣߳��õ�ǰ��������ֱ�ӽ���
	std::unique_lock<std::mutex> locker(m_mutex); // �� unique_lock ��װһ�»�������locker ����ʱ�Զ�����
	while (nullptr == m_evLoop) // Ҫ��֤���߳�ȡ�� thread->evLoop ��Դʱ��������ֻ�û��ʼ����Ϊ NULL �����
	{
		m_cond.wait(locker); // evLoop ��ʼ�����֮ǰһֱ����,�ڼ��ͷ� m_mutex ��
	}
	// ��Щ���������������Ĳ���ʹ�� workerThreadRun �����ڽ���֮ǰ��evLoop �ض����ɹ���ʼ��
	// �ڶ��߳��У��õ�������Դ����Ҫ�������̼߳��ĳЩ������ĳ���Ⱥ�˳������Ҫ�߳�����������Ҫ�����������ź�
	// locker ����������ʱ���ͷŻ������������ֶ�����
}

// ���̵߳Ļص�����
void WorkerThread::running()
{
	m_mutex.lock(); // �Թ�����Դ thread->evLoop ����
	m_evLoop = new EventLoop(m_name);
	m_mutex.unlock(); // ����
	m_cond.notify_one(); // evLoop �Ѿ��ɹ���ʼ�������ڽ�����̵߳�����
	m_evLoop->run();
}
