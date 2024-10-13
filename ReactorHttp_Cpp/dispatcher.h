#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "log.h"

#include <string>

class EventLoop; // �ȸ��߱���������������

class Dispatcher
{
public:
	Dispatcher(EventLoop* evLoop);
	virtual ~Dispatcher();

	// ���
	virtual int add();
	// ɾ��
	virtual int remove();
	// �޸�
	virtual int modify();
	// �¼����
	virtual int dispatch(int timeout = 2); // ��ʱʱ����λ s
	// ���� m_channel
	inline void setChannel(Channel* channel)
	{
		this->m_channel = channel;
	}

protected:
	std::string m_name = std::string();
	Channel* m_channel;
	EventLoop* m_evLoop;
};




