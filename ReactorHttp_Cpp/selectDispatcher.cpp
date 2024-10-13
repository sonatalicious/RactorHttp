#include "dispatcher.h"

#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "selectDispatcher.h"

void SelectDispatcher::setFdSet()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) // 看看是否注册读事件
	{
		FD_SET(m_channel->getSocket(), &m_readSet); // 把文件描述符设置到读集合中
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)// 看看是否注册写事件
	{
		FD_SET(m_channel->getSocket(), &m_writeSet); // 把文件描述符设置到写集合中
	}
}
void SelectDispatcher::clearFdSet()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
	{
		FD_CLR(m_channel->getSocket(), &m_readSet); // 把文件描述符从读集合中删除(文件描述符对应的标志位置为0)
	}
	// 这里有逻辑错误，没有把 channel->fd 从读集合中去掉
	// 所以这里不检测 channel->events ，不管有没有都删掉这个fd的写事件
	// 或者直接用 FD_ZERO
	//if (channel->events & WriteEvent)
	//{
		FD_CLR(m_channel->getSocket(), &m_writeSet);  // 把文件描述符从写集合中删除
	//}
}

SelectDispatcher::SelectDispatcher(EventLoop* evLoop): Dispatcher(evLoop)
{
	FD_ZERO(&m_readSet); // 用宏函数设置集合为 0
	FD_ZERO(&m_writeSet);
	m_name = "Select";
}

SelectDispatcher::~SelectDispatcher()
{
}

int SelectDispatcher::add()
{
	if (m_channel->getSocket() >= m_maxSize)
	{
		return -1;
	}
	setFdSet();
	return 0;
}

int SelectDispatcher::remove()
{
 	if (m_channel->getSocket() >= m_maxSize)
	{
		return -1;
	}
	clearFdSet();
	// 通过 channel 注册的回调函数，释放相对应的 tcpConnection 资源
	m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
	return 0;
}

int SelectDispatcher::modify()
{
	clearFdSet();
	setFdSet();
	return 0;
}

int SelectDispatcher::dispatch(int timeout)
{
	struct timeval val;
	val.tv_sec = timeout;
	val.tv_usec = 0;
	fd_set rdtmp = m_readSet;
	fd_set wrtmp = m_writeSet;
	int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val);
	if (-1 == count)
	{
		Error("select -- errno:%d -- errnoMsg:%s", errno, strerror(errno));
		exit(0);
	}
	for (int i = 0; i < m_maxSize; ++i) // 这个 i 相当于就是 fd
	{
		if (FD_ISSET(i, &rdtmp)) // 读事件被激活
		{
			m_evLoop->eventActive(i, (int)FDEvent::ReadEvent);
		}
		if (FD_ISSET(i, &wrtmp)) // 写事件被激活
		{
			m_evLoop->eventActive(i, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}
