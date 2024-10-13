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

	// 添加
	int add() override;
	// 删除
	int remove() override;
	// 修改
	int modify() override;
	// 事件检测
	int dispatch(int timeout = 2) override; // 超时时常单位 s

private:
	void setFdSet();
	void clearFdSet();

private:
	fd_set m_readSet;
	fd_set m_writeSet;
	const int m_maxSize = 1024;
};





