#pragma once

#include <pthread.h>

#include "dispatcher.h"
#include "channelMap.h"

extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;

// ����ýڵ��е� channel �ķ�ʽ
enum ElemType{ADD, DELETE, MODIFY};
// ����������еĽڵ�
struct ChannelElement
{
	int type; // ��δ���ýڵ��е� channel
	struct Channel* channel;
	struct ChannelElement* next;
};

struct Dispatcher; // �ȸ��߱���������������

struct EventLoop
{
	bool isQuit;
	struct Dispatcher* dispatcher;
	void* dispatcherData;
	// ������� taskQueue(���̺߳ʹ����̹߳��ã�����װ��ν��У�
	struct ChannelElement* head;
	struct ChannelElement* tail;
	// channel map
	struct ChannelMap* channelMap;
	// �߳�id��name�������� mutex(��Ҫ�����������)
	pthread_t threadID;
	char threadName[32];
	pthread_mutex_t mutex;
	// �洢����ͨ�ŵ� fd��ͨ�� socketpair ��ʼ��
	int socketPair[2];
};

// ��ʼ��
struct EventLoop* eventLoopInit(); // ��ʼ�����߳�
struct EventLoop* eventLoopInitEx(const char* threadName); // ��ʼ�����߳�
// ������Ӧ��ģ��
int eventLoopRun(struct EventLoop* evLoop);
// ��������� fd
int eventActivate(struct EventLoop* evLoop, int fd, int event);
// ��������������
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type);
// ������������е�����
int eventLoopProcessTask(struct EventLoop* evLoop);
// ���� dispatcher �еĽڵ�(������������еĽڵ㣩����ɾ�ļ��� fd
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
// �ͷ� channel
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel);
