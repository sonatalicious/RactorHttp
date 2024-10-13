#include "tcpConnection.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>

int processRead(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	// 接收数据
	int count = bufferSocketRead(conn->readBuf, conn->channel->fd);
	Debug("连接：%s 接收到的 http 请求数据：\n%s",conn->name, conn->readBuf->data + conn->readBuf->readPos);
	if (count > 0)
	{
		// 接收到了 http 请求，解析 http 请求
		int socket = conn->channel->fd;
#ifdef MSG_SEND_AUTO
		// 修改 channel 并添加 task ，让 dispatcher 检测写事件
		writeEventEnable(conn->channel, true);
		eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif
		bool flag = parseHttpRequest(conn->request, conn->readBuf,conn->response, conn->writeBuf, socket);
		if (!flag)
		{
			// 解析失败，回复一个简单的 html
			char* errMsg = "Http/1.1 400 Bad Requset\r\n\r\n";
			bufferAppendString(conn->writeBuf, errMsg);
		}
	}
	else // 直接断开连接
	{
#ifdef MSG_SEND_AUTO
		// 断开连接
		eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	}
#ifndef MSG_SEND_AUTO
	// 断开连接
	eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
}

// 这个函数在 MSG_SEND_AUTO 不被定义时，实质上是不会被调用的，所以可以不注释
int processWrite(void* arg)
{
	Debug("开始发送数据了（基于写事件发送）...");
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	// 发送数据
	int count = bufferSendData(conn->writeBuf, conn->channel->fd);
	if (count > 0)
	{
		// 判断数据是否被全发送出去了
		if (0 == bufferReadableSize(conn->writeBuf))
		{
			// 注意，下面的 1. 2. 3. 代码，如果服务器在通信完成后想要断开连接
			// 那 1. 2. 的代码可写可不写，写不写都行
			// 1. 不再检测写事件 -- 修改 channel 中保存的事件
			// 
			// 不知道为什么，这行代码加上后，SelectDispatcher 会出错
			// edit -- 找到问题了，有逻辑错误，select 的读集合在这个逻辑下，
			// 没有把相关的 fd 设置掉，详见 clearFdSet
			writeEventEnable(conn->channel, false); 
			// 2. 修改 dispatcher 检测的集合 -- 添加任务节点
			eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
			// 3. 删除这个节点
			eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
		}
	}
}

struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop)
{
	struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
	conn->evLoop = evLoop;
	conn->readBuf = bufferInit(10240); // 10k
	conn->writeBuf = bufferInit(10240); // 10k
	// http
	conn->request = httpRequestInit();
	conn->response = httpResponseInit();
	sprintf(conn->name, "Connection-%d", fd);
	// 注意这里只检测读事件，因为写事件一直是可用的，需要检测写事件时再修改
	conn->channel = channelInit(fd, ReadEvent, processRead, processWrite, tcpConnectionDestroy, conn);
	eventLoopAddTask(evLoop, conn->channel, ADD);
	Debug("和客户端建立连接，threadName:%s，threadID:%s，conName:%s",
		evLoop->threadName,evLoop->threadID,conn->name);
	return conn;
}

int tcpConnectionDestroy(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	if (NULL != conn)
	{
		// 当 readBuf 和 writeBuf 都指向有效内存，并且没有可处理的数据时
		if (conn->readBuf && bufferReadableSize(conn->readBuf) == 0 &&
			conn->writeBuf && bufferReadableSize(conn->writeBuf) == 0)
		{
			Debug("连接断开，释放资源，connName:%s", conn->name);
			destroyChannel(conn->evLoop, conn->channel);
			bufferDestroy(conn->readBuf);
			bufferDestroy(conn->writeBuf);
			httpRequestDestroy(conn->request);
			httpResponseDestroy(conn->response);
			free(conn);
		}
	}
	return 0;
}
