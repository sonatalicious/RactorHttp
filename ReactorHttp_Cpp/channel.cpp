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
		//this->m_events |= (int)FDEvent::WriteEvent; // C ���Է��
		this->m_events |= static_cast<int>(FDEvent::WriteEvent); // C++ 11 ������ǿ������ת��
	}
	else
	{
		this->m_events = this->m_events & ~static_cast<int>(FDEvent::WriteEvent);
	}
}

bool Channel::isWriteEventEnable()
{
	// ��������õ�����0����(true)������Ϊ0(false)
	return this->m_events & static_cast<int>(FDEvent::WriteEvent); 
}
