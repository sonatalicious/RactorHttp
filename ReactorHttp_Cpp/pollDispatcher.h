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

	// 添加
	int add() override;
	// 删除
	int remove() override;
	// 修改
	int modify() override;
	// 事件检测
	int dispatch(int timeout = 2) override; // 超时时常单位 s

private:
	int m_maxfd;
	struct pollfd* m_fds;
	const int m_maxNode = 1024;

};
