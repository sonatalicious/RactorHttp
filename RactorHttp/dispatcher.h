#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "log.h"

struct EventLoop; // �ȸ��߱���������������

struct Dispatcher
{
	// init -- ��ʼ�� epoll��poll ���� select ��Ҫ�����ݿ�
	void* (*init)();
	// ���
	int (*add)(struct Channel* channel,struct EventLoop* evLoop);
	// ɾ��
	int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
	// �޸�
	int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
	// �¼����
	int (*dispatch)(struct EventLoop* evLoop, int timeout); // ��ʱʱ����λ s
	// ������ݣ��ر�fd�����ͷ��ڴ棩
	int (*clear)(struct EventLoop* evLoop);
};




