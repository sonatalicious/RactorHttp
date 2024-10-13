#include "tcpServer.h"
#include "tcpConnection.h"
#include "log.h"

#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>

TcpServer::TcpServer(unsigned short port, int threadNum)
{
	m_port = port;
	m_mainLoop = new EventLoop;
	m_threadNum = threadNum;
	m_threadPool = new ThreadPool(m_mainLoop, threadNum);
	setListen();
}

TcpServer::~TcpServer()
{
	// 其实这个析构函数可写可不写，这里还是写上了
	delete m_mainLoop;
	delete m_threadPool;
}

void TcpServer::setListen()
{
	// 1.创建监听fd
	m_lfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET 表示网络层协议是 ipv4，SOCK_STREAM 表示传输层是流式协议
	if (-1 == m_lfd)
	{
		Error("socket\n");
		return;
	}
	// 2.设置端口复用
	int opt = 1;
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (-1 == ret)
	{
		Error("setsockopt\n");
		return;
	}
	// 3.绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(m_lfd, (struct sockaddr*)&addr, sizeof addr);
	if (-1 == ret)
	{
		Error("bind\n");
		return;
	}
	// 4.设置监听
	ret = listen(m_lfd, 128);
	if (-1 == ret)
	{
		Error("listen\n");
		return;
	}
}

void TcpServer::run()
{
	// 启动线程池
	m_threadPool->run();
	// 添加检测的任务
	// 初始化一个 channel 实例
	Channel* channel = new Channel(m_lfd,\
		FDEvent::ReadEvent, TcpServer::acceptConnection, nullptr, nullptr, this);

	m_mainLoop->addTask(channel, ElemType::ADD); // 为主线程的 eventLoop 添加
	// 启动反应堆模型
	Debug("服务器程序已经启动了...监听端口为 : %d", m_port);
	m_mainLoop->run();
}

// 这个函数只会在主线程中被调用
int TcpServer::acceptConnection(void* arg)
{
	// 静态函数属于类，不能直接访问实例，所以这里把实例传进来再转换一下
	TcpServer* server = static_cast<TcpServer*>(arg);
	// 和客户端建立连接
	int cfd = accept(server->m_lfd, NULL, NULL);
	// 从线程池中取出一个子线程的反应堆实例，去处理这个cfd
	EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();
	// 将 cfd 放到 tcpConnection 中处理
	// 这个 conn 没有被使用，可以不出现的
	TcpConnection* conn = new TcpConnection(cfd, evLoop);
	return 0;
}
