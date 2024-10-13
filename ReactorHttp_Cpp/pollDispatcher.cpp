#include "dispatcher.h"
#include "log.h"

#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "pollDispatcher.h"

PollDispatcher::PollDispatcher(EventLoop* evLoop): Dispatcher(evLoop)
{
	m_maxfd = 0;
	m_fds = new struct pollfd[m_maxNode];
	for (int i = 0; i < m_maxNode; ++i)
	{
		m_fds[i].fd = -1;
		m_fds[i].events = 0;
		m_fds[i].revents = 0;
	}
	m_name = "Poll";
}

PollDispatcher::~PollDispatcher()
{
	delete[] m_fds;
}

int PollDispatcher::add()
{
	// 先拿到 eventloop 中的 data 数据 data
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) // 看看是否注册读事件
	{
		events |= POLLIN;
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)// 看看是否注册写事件
	{
		events |= POLLOUT;
	}
	int i = 0;
	for (; i < m_maxNode; ++i)
	{
		if (-1 == m_fds[i].fd) // 找到第一个空闲的没有被占用的元素
		{
			m_fds[i].events = events;
			m_fds[i].fd = m_channel->getSocket();
			m_maxfd = (i > m_maxfd) ? i : m_maxfd;
			break;
		}
	}
	if (i >= m_maxNode)
	{
		return -1;
	}
	return 0;
}

int PollDispatcher::remove()
{
	int i = 0;
	for (; i < m_maxNode; ++i)
	{
		if (m_fds[i].fd == m_channel->getSocket()) // 找到此文件描述符在 pollfd 数组中的位置
		{
			m_fds[i].events = 0;
			m_fds[i].revents = 0;
			m_fds[i].fd = -1;
			break;
		}
	}
	// 通过 channel 注册的回调函数，释放相对应的 tcpConnection 资源
	m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
	if (i >= m_maxNode)
	{
		return -1;
	}
	return 0;
}

int PollDispatcher::modify()
{
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) // 看看是否注册读事件
	{
		events |= POLLIN;
	}
	if (m_channel->getEvent()& (int)FDEvent::WriteEvent)// 看看是否注册写事件
	{
		events |= POLLOUT;
	}
	int i = 0;
	for (; i < m_maxNode; ++i)
	{
		if (m_fds[i].fd == m_channel->getSocket()) // 找到此文件描述符在 pollfd 数组中的位置
		{
			m_fds[i].events = events;
			break;
		}
	}
	if (i >= m_maxNode)
	{
		return -1;
	}
	return 0;
}

int PollDispatcher::dispatch(int timeout)
{
	int count = poll(m_fds, m_maxfd + 1, timeout * 1000); // 作用类似于 epoll_wait
	if (-1 == count)
	{
		Error("poll");
		exit(0);
	}
	for (int i = 0; i <= m_maxfd; ++i)
	{
		if (-1 == m_fds[i].fd) // 跳过哪些没有用的 fd
		{
			continue;
		}
		if (m_fds[i].revents & POLLERR || m_fds[i].revents & POLLHUP)
		{
			// 对方断开了连接，删除 fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (m_fds[i].revents & POLLIN) // 读事件被触发
		{
			m_evLoop->eventActive(m_fds[i].fd, (int)FDEvent::ReadEvent);

		}
		if (m_fds[i].revents & POLLOUT) // 写事件被触发
		{
			m_evLoop->eventActive(m_fds[i].fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}
