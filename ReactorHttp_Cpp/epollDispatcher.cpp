#include "dispatcher.h"
#include "log.h"
#include "epollDispatcher.h"

#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

EpollDispatcher::EpollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop)
{
	m_epfd = epoll_create(10);
	if (-1 == m_epfd)
	{
		Error("epoll_create");
		exit(0);
	}
	m_events = new struct epoll_event[m_maxNode];
	m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher()
{
	close(m_epfd);
	delete[] m_events;
}

int EpollDispatcher::add()
{
	int ret = epollCtl(EPOLL_CTL_ADD);
	if (-1 == ret)
	{
		Error("EPOLL_CTL add");
		exit(0);
	}
	return ret;
}

int EpollDispatcher::remove()
{
	int ret = epollCtl(EPOLL_CTL_DEL);
	if (-1 == ret)
	{
		Error("EPOLL_CTL delete");
		exit(0);
	}
	// 通过 channel 注册的回调函数，释放相对应的 tcpConnection 资源
	// const_cast 去掉参数的只读属性
	m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
	return ret;
}

int EpollDispatcher::modify()
{
	int ret = epollCtl(EPOLL_CTL_MOD);
	if (-1 == ret)
	{
		Error("EPOLL_CTL modify");
		exit(0);
	}
	return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
	int count = epoll_wait(m_epfd, m_events, m_maxNode, timeout * 1000);
	for (int i = 0; i < count; ++i)
	{
		int events = m_events[i].events;
		int fd = m_events[i].data.fd;
		if (events & EPOLLERR || events & EPOLLHUP)
		{
			// 对方断开了连接，删除 fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN) // 读事件被触发
		{
			m_evLoop->eventActive(fd, (int)FDEvent::ReadEvent);
		}
		if (events & EPOLLOUT) // 写事件被触发
		{
			m_evLoop->eventActive(fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}

int EpollDispatcher::epollCtl(int op)
{
	struct epoll_event ev;
	ev.data.fd = m_channel->getSocket();
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) // 看看是否注册读事件
	{
		events |= EPOLLIN;
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)// 看看是否注册写事件
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
	return ret;
}
