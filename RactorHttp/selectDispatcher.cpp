#include "dispatcher.h"

#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX 1024

struct SelectData
{
	fd_set readSet;
	fd_set writeSet;
};

static void* selectInit();
static int selectAdd(struct Channel* channel, struct EventLoop* evLoop);
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop);
static int selectModify(struct Channel* channel, struct EventLoop* evLoop);
static int selectDispatch(struct EventLoop* evLoop, int timeout);
static int selectClear(struct EventLoop* evLoop);

static void setFdSet(struct Channel* channel, struct SelectData* data);
static void clearFdSet(struct Channel* channel, struct SelectData* data);

struct Dispatcher SelectDispatcher = {
	selectInit,
	selectAdd,
	selectRemove,
	selectModify,
	selectDispatch,
	selectClear
};

static void* selectInit()
{
	struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
	FD_ZERO(&data->readSet); // �ú꺯�����ü���Ϊ 0
	FD_ZERO(&data->writeSet);
	return data;
}

static void setFdSet(struct Channel* channel, struct SelectData* data)
{
	if (channel->events & ReadEvent) // �����Ƿ�ע����¼�
	{
		FD_SET(channel->fd, &data->readSet); // ���ļ����������õ���������
	}
	if (channel->events & WriteEvent)// �����Ƿ�ע��д�¼�
	{
		FD_SET(channel->fd, &data->writeSet); // ���ļ����������õ�д������
	}
}
static void clearFdSet(struct Channel* channel, struct SelectData* data)
{
	if (channel->events & ReadEvent)
	{
		FD_CLR(channel->fd, &data->readSet); // ���ļ��������Ӷ�������ɾ��(�ļ���������Ӧ�ı�־λ��Ϊ0)
	}
	// �������߼�����û�а� channel->fd �Ӷ�������ȥ��
	// �������ﲻ��� channel->events ��������û�ж�ɾ�����fd��д�¼�
	// ����ֱ���� FD_ZERO
	//if (channel->events & WriteEvent)
	//{
		FD_CLR(channel->fd, &data->writeSet);  // ���ļ���������д������ɾ��
	//}
}

static int selectAdd(struct Channel* channel, struct EventLoop* evLoop)
{
	// ���õ� eventloop �е� data ���� data
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	if (channel->fd >= MAX)
	{
		return -1;
	}
	//if (channel->events & ReadEvent) // �����Ƿ�ע����¼�
	//{
	//	FD_SET(channel->fd, &data->readSet); // ���ļ����������õ���������
	//}
	//if (channel->events & WriteEvent)// �����Ƿ�ע��д�¼�
	//{
	//	FD_SET(channel->fd, &data->writeSet); // ���ļ����������õ�д������
	//}
	setFdSet(channel, data);
	return 0;
}

static int selectRemove(struct Channel* channel, struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	if (channel->fd >= MAX)
	{
		return -1;
	}
	//if (channel->events & ReadEvent)
	//{
	//	FD_CLR(channel->fd, &data->readSet); // ���ļ��������Ӷ�������ɾ��(�ļ���������Ӧ�ı�־λ��Ϊ0)
	//}
	//if (channel->events & WriteEvent)
	//{
	//	FD_CLR(channel->fd, &data->writeSet);  // ���ļ���������д������ɾ��
	//}
	clearFdSet(channel, data);
	// ͨ�� channel ע��Ļص��������ͷ����Ӧ�� tcpConnection ��Դ
	channel->destroyCallback(channel->arg);
	return 0;
}

static int selectModify(struct Channel* channel, struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	clearFdSet(channel, data);
	setFdSet(channel, data);
	return 0;
}

#if 0
void print_fds_in_set(const fd_set* fds, int max_fd) {
	printf("File descriptors in set: ");
	for (int fd = 0; fd < max_fd; ++fd) {
		if (FD_ISSET(fd, fds)) {
			printf("%d ", fd);
		}
	}
	printf("\n");
}
#endif

static int selectDispatch(struct EventLoop* evLoop, int timeout)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	struct timeval val;
	val.tv_sec = timeout;
	val.tv_usec = 0;
	fd_set rdtmp = data->readSet;
	fd_set wrtmp = data->writeSet;
	int count = select(MAX, &rdtmp, &wrtmp, NULL, &val);
	if (-1 == count)
	{
#if 0
		print_fds_in_set(&rdtmp,MAX);
		print_fds_in_set(&wrtmp, MAX);
#endif
		Error("select -- errno:%d -- errnoMsg:%s",errno,strerror(errno));
		exit(0);
	}
	for (int i = 0; i < MAX; ++i) // ��� i �൱�ھ��� fd
	{
		if (FD_ISSET(i, &rdtmp)) // ���¼�������
		{
			eventActivate(evLoop, i, ReadEvent);
		}
		if (FD_ISSET(i, &wrtmp)) // д�¼�������
		{
			eventActivate(evLoop, i, WriteEvent);
		}
	}
	return 0;
}
static int selectClear(struct EventLoop* evLoop)
{
	struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
	free(data);
	return 0;
}
