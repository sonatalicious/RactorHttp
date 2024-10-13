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
	// ͨ�� channel ע��Ļص��������ͷ����Ӧ�� tcpConnection ��Դ
	// const_cast ȥ��������ֻ������
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
			// �Է��Ͽ������ӣ�ɾ�� fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN) // ���¼�������
		{
			m_evLoop->eventActive(fd, (int)FDEvent::ReadEvent);
		}
		if (events & EPOLLOUT) // д�¼�������
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
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) // �����Ƿ�ע����¼�
	{
		events |= EPOLLIN;
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)// �����Ƿ�ע��д�¼�
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
	return ret;
}
