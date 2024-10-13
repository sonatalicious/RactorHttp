#include "eventLoop.h"
#include "log.h"
#include "selectDispatcher.h"
#include "pollDispatcher.h"
#include "epollDispatcher.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <string>

EventLoop::EventLoop() : EventLoop(std::string())
{ // ����һ��ί�й��캯����C++11�����ԣ���ע�ⲻҪ����ί��
}

EventLoop::EventLoop(const std::string threadName)
{
	m_isQuit = true; // Ĭ��û������
	m_threadID = std::this_thread::get_id();
	m_threadName = threadName == std::string() ? "MainThread" : threadName;
	m_dispatcher = new SelectDispatcher(this);
	// channelMap
	m_channelMap.clear();
	// ��ʼ������ͨ�ŵ��ڹ� fd
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
	if (-1 == ret)
	{
		Error("socketpair");
		exit(0);
	}
#if 0
	// ����ڹ� fd
	// ָ������ evLoop->socketPair[0] �������ݣ� evLoop->socketPair[1] ��������
	Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, \
		readLocalMessage, nullptr, nullptr, this);
#else
	// �� bind -- C++11��Ŀɵ��ö������
	// ����ķǾ�̬��Ա������
	auto obj = std::bind(&EventLoop::readMessage, this);
	Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, \
		obj, nullptr, nullptr, this);
#endif
	// channel ��ӵ�������У���� fd ��ʵ�����ǵ���Ӧ�����߳��򱾵� fd ͨ�ţ���������������/���߳�
	addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop() // ��������������Ҫ�������
{
}

int EventLoop::run()
{
	m_isQuit = false;
	// �Ƚ��߳�ID�Ƿ�����
	if (m_threadID != std::this_thread::get_id())
	{
		Error("not this thread!");
		return -1;
	}
	// ѭ�������¼�����
	while (!m_isQuit)
	{
		m_dispatcher->dispatch(); // ��ʱʱ�� 2s
		processTaskQ(); // ����һ���������
	}
	return 0;
}

int EventLoop::eventActive(int fd, int event)
{
	if (fd < 0)
	{
		return -1;
	}
	// ȡ�� channel
	Channel* channel = m_channelMap[fd];
	assert(channel->getSocket() == fd);
	if (event & (int)FDEvent::ReadEvent && channel->readCallback) // ����Ƕ��¼�������ص�������Ϊ��
	{
		channel->readCallback(const_cast<void*>(channel->getArg())); // ������ض��Ļص�����
	}
	if (event & (int)FDEvent::WriteEvent && channel->writeCallback) // �����д�¼�������ص�������Ϊ��
	{
		channel->writeCallback(const_cast<void*>(channel->getArg())); // �������д�Ļص�����
	}
	return 0;
}

int EventLoop::addTask(Channel* channel, ElemType type)
{
	// ���̺߳� eventloop �̶߳����������ڴ棬����Ҫ����������������Դ
	m_mutex.lock();
	// �����½ڵ�
	ChannelElement* node = new ChannelElement;
	node->channel = channel;
	node->type = type;
	// ���½ڵ�ŵ�������
	m_taskQ.push(node);
	m_mutex.unlock();
	// ����ڵ�
	/*
	ϸ�ڣ�
	eventLoop ��Ϊ��Ӧ�ѣ������̺߳����̶߳���
	1����ǰ�߳�Ϊ���߳�ʱ�������������̣߳����̣߳�����˴�����ڵ�
		1���޸� fd �¼��ǵ�ǰ���̷߳���ģ���ǰ���̴߳���
		2������� fd���������ڵ�Ĳ����������̷߳���ģ��ⲿ�Է�����������һ��������
	2�����������̴߳���������У���Ҫ�ɵ�ǰ�����߳�ȥ����
	*/
	if (std::this_thread::get_id() == m_threadID)
	{
		// ��ǰΪ���̣߳��������̵߳ĽǶȷ�����
		processTaskQ();
	}
	else
	{
		// ��ǰΪ���߳� -- �������߳�ȥ������������е�����
		// ��ʱ�����������
		// 1.���߳��ڹ��� 
		// 2.���̱߳������ˣ�select��poll��epoll
		//��û���¼�ʱ�ײ��������IO��·����ģ�ͻ�������ʱ�������Ϊ 2s��

		// ����Ӧ socket��evLoop->socketPair[1] д����
		// evLoop->socketPair[1] �������ݣ��Դ˻������������߳�
		taskWakeup();
	}
	return 0;
}

int EventLoop::processTaskQ()
{
	// ȡ��ͷ�ڵ�
	while (!m_taskQ.empty())
	{
		// ����������������Դ
		m_mutex.lock();
		ChannelElement* node = m_taskQ.front();
		m_taskQ.pop(); // ɾ���ڵ�
		// �������ͷ���Դ
		m_mutex.unlock();
		Channel* channel = node->channel;
		if (ElemType::ADD == node->type)
		{
			// ���
			add(channel);
		}
		else if (ElemType::DELETE == node->type)
		{
			// ɾ��
			remove(channel);
		}
		else if (ElemType::MODIFY == node->type)
		{
			// �޸�
			modify(channel);
		}
		delete node;
	}
	return 0;
}

int EventLoop::add(Channel* channel)
{
	int fd = channel->getSocket();
	// �ҵ� fd ��Ӧ������Ԫ��λ�ã����洢
	if (m_channelMap.end() == m_channelMap.find(fd)) // ���� fd �ڲ��� m_channelMap ��
	{
		m_channelMap.insert(std::make_pair(fd, channel));
		m_dispatcher->setChannel(channel);
		int ret = m_dispatcher->add();
		return ret;
	}
	return -1;
}

int EventLoop::remove(Channel* channel)
{
	int fd = channel->getSocket();
	if (m_channelMap.end() == m_channelMap.find(fd)) // ���� fd �ڲ��� m_channelMap ��
	{
		return -1;
	}
	m_dispatcher->setChannel(channel);
	int ret = m_dispatcher->remove();
	return ret;
}

int EventLoop::modify(Channel* channel)
{
	int fd = channel->getSocket();
	// ��� fd ���� channelMap ����⼯���У���û�ҵ� fd ��Ӧ�� channel
	if (m_channelMap.end() == m_channelMap.find(fd)) // ���� fd �ڲ��� m_channelMap ��
	{
		return -1;
	}
	m_dispatcher->setChannel(channel);
	int ret = m_dispatcher->modify();
	return ret;
}

int EventLoop::freeChannel(Channel* channel)
{
	auto it = m_channelMap.find(channel->getSocket());
	if (m_channelMap.end() != it) // ���� fd �ڲ��� m_channelMap ��
	{
		m_channelMap.erase(it);
		close(channel->getSocket());
		free(channel);
	}
	return 0;
}

// ����д����
void EventLoop::taskWakeup()
{
	const char* msg = "wake up! (placeholder) �������߳�\n";
	write(m_socketPair[0], msg, strlen(msg));
}

// ���ض�����
// �����Ǿ�̬������������
// ���ܵ������ĳ��ʵ���ĳ�Ա�����Խ�������ʵ��������Ϊ���� arg ����
int EventLoop::readLocalMessage(void* arg)
{
	EventLoop* evLoop = static_cast<EventLoop*>(arg);
	char buf[256];
	read(evLoop->m_socketPair[1], buf, sizeof(buf));
	// �������յ�ʲô�������������
	return 0;
}

// ���ض�����
// ������� readLocalMessage ���Ӧ����Ա�������ڶ���ʵ��������Ҫ��������
int EventLoop::readMessage()
{
	char buf[256];
	read(m_socketPair[1], buf, sizeof(buf));
	// �������յ�ʲô�������������
	return 0;
}
