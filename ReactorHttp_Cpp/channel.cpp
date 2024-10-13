#include "channel.h"

#include <stdlib.h>
#include <stdio.h>

Channel::Channel(int fd, FDEvent events, handleFunc readFunc,\
	handleFunc writeFunc, handleFunc destroyFunc, void* arg)
{
	this->m_arg = arg;
	this->m_fd = fd;
	this->m_events = (int)events;
	this->readCallback = readFunc;
	this->writeCallback = writeFunc;
	this->destroyCallback = destroyFunc;
}

void Channel::writeEventEnable(bool flag)
{
	if (flag)
	{
		//this->m_events |= (int)FDEvent::WriteEvent; // C 语言风格
		this->m_events |= static_cast<int>(FDEvent::WriteEvent); // C++ 11 新特性强制类型转换
	}
	else
	{
		this->m_events = this->m_events & ~static_cast<int>(FDEvent::WriteEvent);
	}
}

bool Channel::isWriteEventEnable()
{
	// 若成立则得到大于0的数(true)，否则为0(false)
	return this->m_events & static_cast<int>(FDEvent::WriteEvent); 
}
