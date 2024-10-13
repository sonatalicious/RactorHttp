#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "dispatcher.h"
#include "log.h"

#include <sys/epoll.h>
#include <string>

class EpollDispatcher : public Dispatcher
{
public:
	EpollDispatcher(EventLoop* evLoop);
	virtual ~EpollDispatcher();

	// ���
	int add() override;
	// ɾ��
	int remove() override;
	// �޸�
	int modify() override;
	// �¼����
	int dispatch(int timeout = 2) override; // ��ʱʱ����λ s

private:
	int epollCtl(int op);

private:
	int m_epfd;
	struct epoll_event* m_events;
	const int m_maxNode = 520;
};





