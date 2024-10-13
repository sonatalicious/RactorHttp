#include "tcpServer.h"
#include "tcpConnection.h"
#include "log.h"

#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>

struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
    struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
    tcp->listener = listenerInit(port);
    tcp->mainLoop = eventLoopInit();
    tcp->threadNum = threadNum;
    tcp->threadPool = threadPoolInit(tcp->mainLoop, threadNum);
    return tcp;
}

struct Listener* listenerInit(unsigned short port)
{
	struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
	// 1.创建监听fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET 表示网络层协议是 ipv4，SOCK_STREAM 表示传输层是流式协议
	if (-1 == lfd)
	{
		Error("socket\n");
		return NULL;
	}
	// 2.设置端口复用
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (-1 == ret)
	{
		Error("setsockopt\n");
		return NULL;
	}
	// 3.绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
	if (-1 == ret)
	{
		Error("bind\n");
		return NULL;
	}
	// 4.设置监听
	ret = listen(lfd, 128);
	if (-1 == ret)
	{
		Error("listen\n");
		return NULL;
	}
	// 返回 listener
	listener->lfd = lfd;
	listener->port = port;
    return listener;
}

int acceptConnection(void* arg)
{
	struct TcpServer* server = (struct TcpServer*)arg;
	// 和客户端建立连接
	int cfd = accept(server->listener->lfd, NULL, NULL);
	// 从线程池中取出一个子线程的反应堆实例，去处理这个cfd
	struct EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
	// 将 cfd 放到 tcpConnection 中处理
	tcpConnectionInit(cfd, evLoop);
}

void tcpServerRun(struct TcpServer* server)
{
	// 启动线程池
	threadPoolRun(server->threadPool);
	// 添加检测的任务
	// 初始化一个 channel 实例
	Debug("服务器监听端口为 : %d", server->listener->port);
	struct Channel* channel = channelInit(server->listener->lfd,
		ReadEvent, acceptConnection, NULL,NULL, server);
	eventLoopAddTask(server->mainLoop, channel, ADD); // 为主线程的 eventLoop 添加
	// 启动反应堆模型
	Debug("服务器程序已经启动了...");
	eventLoopRun(server->mainLoop);
}
