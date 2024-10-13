#include "eventLoop.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

struct EventLoop* eventLoopInit()
{
	return eventLoopInitEx(NULL);
}

// ����д����
void taskWakeup(struct EventLoop* evLoop)
{
	const char* msg = "wake up! (placeholder) �������߳�\n";
	write(evLoop->socketPair[0], msg, strlen(msg));
}
// ���ض�����
int readLocalMessage(void* arg)
{
	struct EventLoop* evLoop = (struct EventLoop*)arg;
	char buf[256];
	read(evLoop->socketPair[1], buf, sizeof(buf));
	// �������յ�ʲô�������������
	return 0;
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
	struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
	evLoop->isQuit = false;
	evLoop->threadID = pthread_self();
	pthread_mutex_init(&evLoop->mutex, NULL);
	strcpy(evLoop->threadName, NULL == threadName ? "MainThread" : threadName);
	evLoop->dispatcher = &SelectDispatcher;
	evLoop->dispatcherData = evLoop->dispatcher->init();
	// ����
	evLoop->head = evLoop->tail = NULL;
	// channelMap
	evLoop->channelMap = channelMapInit(128);
	// ��ʼ������ͨ�ŵ��ڹ� fd
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
	if (-1 == ret)
	{
		Error("socketpair");
		exit(0);
	}
	// ����ڹ� fd
	// ָ������ evLoop->socketPair[0] �������ݣ� evLoop->socketPair[1] ��������
	struct Channel* channel = channelInit(evLoop->socketPair[1], ReadEvent, readLocalMessage, NULL,NULL, evLoop);
	// channel ��ӵ�������У���� fd ��ʵ�����ǵ���Ӧ�����߳��򱾵� fd ͨ�ţ���������������/���߳�
	eventLoopAddTask(evLoop, channel, ADD);
	return evLoop;
}

int eventLoopRun(struct EventLoop* evLoop)
{
	assert(evLoop != NULL);
	// ȡ���¼��ַ��ͼ��ģ��
	struct Dispatcher* dispatcher = evLoop->dispatcher;
	// �Ƚ��߳�ID�Ƿ�����
	if (evLoop->threadID != pthread_self())
	{
		Error("not this thread!");
		return - 1;
	}
	// ѭ�������¼�����
	while (!evLoop->isQuit)
	{
		dispatcher->dispatch(evLoop, 2); // ��ʱʱ�� 2s
		eventLoopProcessTask(evLoop); // ����һ���������
	}
	return 0;
}

int eventActivate(struct EventLoop* evLoop, int fd, int event)
{
	if (NULL == evLoop || fd < 0)
	{
		return -1;
	}
	// ȡ�� channel
	struct Channel* channel = evLoop->channelMap->list[fd];
	assert(channel->fd == fd);
	if (event & ReadEvent && channel->readCallback) // ����Ƕ��¼�������ص�������Ϊ��
	{
		channel->readCallback(channel->arg); // ������ض��Ļص�����
	}
	if (event & WriteEvent && channel->writeCallback) // �����д�¼�������ص�������Ϊ��
	{
		channel->writeCallback(channel->arg); // �������д�Ļص�����
	}
	return 0;
}

int eventLoopAddTask(struct EventLoop* evLoop,struct Channel* channel, int type)
{
	// ���̺߳� eventloop �̶߳����������ڴ棬����Ҫ����������������Դ
	pthread_mutex_lock(&evLoop->mutex);
	// �����½ڵ�
	struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
	node->channel = channel;
	node-> type = type;
	node->next = NULL;
	// ���½ڵ�ŵ�������
	if (NULL == evLoop->head)
	{
		evLoop->head = evLoop->tail = node;
	}
	else
	{
		evLoop->tail->next = node; // ���
		evLoop->tail = node; // tail ����
	}
	pthread_mutex_unlock(&evLoop->mutex); // ����
	// ����ڵ�
	/*
	ϸ�ڣ�
	eventLoop ��Ϊ��Ӧ�ѣ������̺߳����̶߳���
	1����ǰ�߳�Ϊ���߳�ʱ�������������̣߳����̣߳�����˴�����ڵ�
		1���޸� fd �¼��ǵ�ǰ���̷߳���ģ���ǰ���̴߳���
		2������� fd���������ڵ�Ĳ����������̷߳���ģ��ⲿ�Է�����������һ��������
	2�����������̴߳���������У���Ҫ�ɵ�ǰ�����߳�ȥ����
	*/
	if (evLoop->threadID == pthread_self())
	{
		// ��ǰΪ���̣߳��������̵߳ĽǶȷ�����
		eventLoopProcessTask(evLoop);
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
		taskWakeup(evLoop);
	}
	return 0;
}


int eventLoopProcessTask(struct EventLoop* evLoop)
{
	// ����������������Դ
	pthread_mutex_lock(&evLoop->mutex);
	// ȡ��ͷ�ڵ�
	struct ChannelElement* head = evLoop->head;
	while (NULL != head)
	{
		struct Channel* channel = head->channel;
		if (ADD == head->type)
		{
			// ���
			eventLoopAdd(evLoop, channel);
		}
		else if (DELETE == head->type)
		{
			// ɾ��
			eventLoopRemove(evLoop, channel);
		}		
		else if (MODIFY == head->type)
		{
			// �޸�
			eventLoopModify(evLoop, channel);
		}
		struct ChannelElement* tmp = head;
		head = head->next;
		free(tmp);
	}
	// ���񶼱��������ˣ����³�ʼ��һ��
	evLoop->head = evLoop->tail = NULL;
	// �������ͷ���Դ
	pthread_mutex_unlock(&evLoop->mutex);
	return 0;
}

int eventLoopAdd(EventLoop* evLoop, Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size)
	{
		// �� channelmap û�пռ�洢�� fd ��Ӧ�� channel ʱ ==> ����
		if (!makeMapRoom(channelMap, fd, sizeof(struct Channel*)))
		{
			// ����ʧ��
			return -1;
		}
	}
	// �ҵ� fd ��Ӧ������Ԫ��λ�ã����洢
	if (NULL == channelMap->list[fd])
	{
		channelMap->list[fd] = channel;
		evLoop->dispatcher->add(channel, evLoop);
	}
	return 0;
}
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size) // ��� fd ���� channelMap ����⼯����
	{
			return -1;
	}
	int ret = evLoop->dispatcher->remove(channel, evLoop);
	return ret;
}
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	// ��� fd ���� channelMap ����⼯���У���û�ҵ� fd ��Ӧ�� channel
	if (fd >= channelMap->size || NULL == channelMap->list[fd]) 
	{
		return -1;
	}
	int ret = evLoop->dispatcher->modify(channel, evLoop);
	return ret;
}
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel)
{
	// ɾ�� channel �� fd �Ķ�Ӧ��ϵ
	evLoop->channelMap->list[channel->fd] = NULL;
	// �ر� fd
	close(channel->fd);
	// �ͷ� channel
	free(channel);
	return 0;
}