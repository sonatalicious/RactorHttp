#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "dispatcher.h"
#include "log.h"

#include <sys/select.h>
#include <string>

class SelectDispatcher : public Dispatcher
{
public:
	SelectDispatcher(EventLoop* evLoop);
	virtual ~SelectDispatcher();

	// ���
	int add() override;
	// ɾ��
	int remove() override;
	// �޸�
	int modify() override;
	// �¼����
	int dispatch(int timeout = 2) override; // ��ʱʱ����λ s

private:
	void setFdSet();
	void clearFdSet();

private:
	fd_set m_readSet;
	fd_set m_writeSet;
	const int m_maxSize = 1024;
};





