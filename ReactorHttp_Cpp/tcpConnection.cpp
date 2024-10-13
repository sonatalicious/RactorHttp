#include "tcpConnection.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>

int TcpConnection::processRead(void* arg)
{
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	int socket = conn->m_channel->getSocket();
	// 接收数据
	int count = conn->m_readBuf->socketRead(socket);
	Debug("连接：%s 接收到的 http 请求数据：\n%s", conn->m_name.data(), conn->m_readBuf->data());
	if (count > 0)
	{
		// 接收到了 http 请求，解析 http 请求
#ifdef MSG_SEND_AUTO
		// 修改 channel 并添加 task ，让 dispatcher 检测写事件
		conn->m_channel->writeEventEnable(true);
		conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
#endif
		bool flag = conn->m_request->parseHttpRequest(\
			conn->m_readBuf, conn->m_response, conn->m_writeBuf, socket);
		if (!flag)
		{
			// 解析失败，回复一个简单的 html
			std::string errMsg = "Http/1.1 400 Bad Requset\r\n\r\n";
			conn->m_writeBuf->appendString(errMsg);
		}
	}
	else // 直接断开连接
	{
#ifdef MSG_SEND_AUTO
		// 断开连接
		conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
	}
#ifndef MSG_SEND_AUTO
	// 断开连接
	conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
}

// 这个函数在 MSG_SEND_AUTO 不被定义时，实质上是不会被调用的，所以可以不注释
int TcpConnection::processWrite(void* arg)
{
	Debug("开始发送数据了（基于写事件发送）...");
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// 发送数据
	int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
	if (count > 0)
	{
		// 判断数据是否被全发送出去了
		if (0 == conn->m_writeBuf->readableSize())
		{
			// 注意，下面的 1. 2. 3. 代码，如果服务器在通信完成后想要断开连接
			// 那 1. 2. 的代码可写可不写，写不写都行

			// 1. 不再检测写事件 -- 修改 channel 中保存的事件
			// 
			// 不知道为什么，这行代码加上后，SelectDispatcher 会出错
			// edit -- 找到问题了，有逻辑错误，select 的读集合在这个逻辑下，
			// 没有把相关的 fd 设置掉，详见 clearFdSet
			conn->m_channel->writeEventEnable(false);
			// 2. 修改 dispatcher 检测的集合 -- 添加任务节点
			conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
			// 3. 删除这个节点
			conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
		}
	}
}

int TcpConnection::destroy(void* arg)
{
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	if (nullptr != conn)
	{
		delete conn;
	}
	return 0;
}

TcpConnection::TcpConnection(int fd, EventLoop* evLoop)
{
	m_evLoop = evLoop;
	m_readBuf = new Buffer(10240); // 10k
	m_writeBuf = new Buffer(10240); // 10k
	// http
	m_request = new HttpRequest;
	m_response = new HttpResponse;
	m_name = "Connection-" + std::to_string(fd);
	// 注意这里只检测读事件，因为写事件一直是可用的，需要检测写事件时再修改
	m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, \
		processWrite, destroy, this);
	evLoop->addTask(m_channel, ElemType::ADD);
	Debug("和客户端建立连接，threadID:%d，conName:%s",
		evLoop->getThreadID(), m_name.data());
}

TcpConnection::~TcpConnection()
{
	// 当 readBuf 和 writeBuf 都指向有效内存，并且没有可处理的数据时
	if (m_readBuf && m_readBuf->readableSize() == 0 &&
		m_writeBuf && m_writeBuf->readableSize() == 0)
	{
		m_evLoop->freeChannel(m_channel);
		delete m_readBuf;
		delete m_writeBuf;
		delete m_request;
		delete m_response;
	}
	Debug("连接断开，释放资源，connName:%s", m_name.data());
}
