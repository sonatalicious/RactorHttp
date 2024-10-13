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
	// ���õ� eventloop �е� data ���� data
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) // �����Ƿ�ע����¼�
	{
		events |= POLLIN;
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)// �����Ƿ�ע��д�¼�
	{
		events |= POLLOUT;
	}
	int i = 0;
	for (; i < m_maxNode; ++i)
	{
		if (-1 == m_fds[i].fd) // �ҵ���һ�����е�û�б�ռ�õ�Ԫ��
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
		if (m_fds[i].fd == m_channel->getSocket()) // �ҵ����ļ��������� pollfd �����е�λ��
		{
			m_fds[i].events = 0;
			m_fds[i].revents = 0;
			m_fds[i].fd = -1;
			break;
		}
	}
	// ͨ�� channel ע��Ļص��������ͷ����Ӧ�� tcpConnection ��Դ
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
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) // �����Ƿ�ע����¼�
	{
		events |= POLLIN;
	}
	if (m_channel->getEvent()& (int)FDEvent::WriteEvent)// �����Ƿ�ע��д�¼�
	{
		events |= POLLOUT;
	}
	int i = 0;
	for (; i < m_maxNode; ++i)
	{
		if (m_fds[i].fd == m_channel->getSocket()) // �ҵ����ļ��������� pollfd �����е�λ��
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
	int count = poll(m_fds, m_maxfd + 1, timeout * 1000); // ���������� epoll_wait
	if (-1 == count)
	{
		Error("poll");
		exit(0);
	}
	for (int i = 0; i <= m_maxfd; ++i)
	{
		if (-1 == m_fds[i].fd) // ������Щû���õ� fd
		{
			continue;
		}
		if (m_fds[i].revents & POLLERR || m_fds[i].revents & POLLHUP)
		{
			// �Է��Ͽ������ӣ�ɾ�� fd
			//epollRemove(Channel, evLoop);
			continue;
		}
		if (m_fds[i].revents & POLLIN) // ���¼�������
		{
			m_evLoop->eventActive(m_fds[i].fd, (int)FDEvent::ReadEvent);

		}
		if (m_fds[i].revents & POLLOUT) // д�¼�������
		{
			m_evLoop->eventActive(m_fds[i].fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}
