#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "log.h"

#include <string>

class EventLoop; // 先告诉编译器有这种类型

class Dispatcher
{
public:
	Dispatcher(EventLoop* evLoop);
	virtual ~Dispatcher();

	// 添加
	virtual int add();
	// 删除
	virtual int remove();
	// 修改
	virtual int modify();
	// 事件检测
	virtual int dispatch(int timeout = 2); // 超时时常单位 s
	// 更新 m_channel
	inline void setChannel(Channel* channel)
	{
		this->m_channel = channel;
	}

protected:
	std::string m_name = std::string();
	Channel* m_channel;
	EventLoop* m_evLoop;
};




