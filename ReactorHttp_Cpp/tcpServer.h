#pragma once

#include "eventLoop.h"
#include "threadPool.h"

class TcpServer
{
public:
	TcpServer(unsigned short port, int threadNum);
	~TcpServer();
	// ��ʼ������
	void setListen();
	// ����������
	void run();
	static int acceptConnection(void* arg);

private:
	int m_threadNum;
	EventLoop* m_mainLoop;
	ThreadPool* m_threadPool;
	int m_lfd;
	unsigned short m_port;
};

