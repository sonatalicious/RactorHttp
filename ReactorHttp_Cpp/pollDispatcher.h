#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "dispatcher.h"
#include "log.h"

#include <poll.h>
#include <string>

class PollDispatcher : public Dispatcher
{
public:
	PollDispatcher(EventLoop* evLoop);
	virtual ~PollDispatcher();

	// ���
	int add() override;
	// ɾ��
	int remove() override;
	// �޸�
	int modify() override;
	// �¼����
	int dispatch(int timeout = 2) override; // ��ʱʱ����λ s

private:
	int m_maxfd;
	struct pollfd* m_fds;
	const int m_maxNode = 1024;

};
