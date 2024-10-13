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
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) // �����Ƿ�ע����¼�
	{
		FD_SET(m_channel->getSocket(), &m_readSet); // ���ļ����������õ���������
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)// �����Ƿ�ע��д�¼�
	{
		FD_SET(m_channel->getSocket(), &m_writeSet); // ���ļ����������õ�д������
	}
}
void SelectDispatcher::clearFdSet()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
	{
		FD_CLR(m_channel->getSocket(), &m_readSet); // ���ļ��������Ӷ�������ɾ��(�ļ���������Ӧ�ı�־λ��Ϊ0)
	}
	// �������߼�����û�а� channel->fd �Ӷ�������ȥ��
	// �������ﲻ��� channel->events ��������û�ж�ɾ�����fd��д�¼�
	// ����ֱ���� FD_ZERO
	//if (channel->events & WriteEvent)
	//{
		FD_CLR(m_channel->getSocket(), &m_writeSet);  // ���ļ���������д������ɾ��
	//}
}

SelectDispatcher::SelectDispatcher(EventLoop* evLoop): Dispatcher(evLoop)
{
	FD_ZERO(&m_readSet); // �ú꺯�����ü���Ϊ 0
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
	// ͨ�� channel ע��Ļص��������ͷ����Ӧ�� tcpConnection ��Դ
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
	for (int i = 0; i < m_maxSize; ++i) // ��� i �൱�ھ��� fd
	{
		if (FD_ISSET(i, &rdtmp)) // ���¼�������
		{
			m_evLoop->eventActive(i, (int)FDEvent::ReadEvent);
		}
		if (FD_ISSET(i, &wrtmp)) // д�¼�������
		{
			m_evLoop->eventActive(i, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}
