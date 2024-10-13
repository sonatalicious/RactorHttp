#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "log.h"

struct EventLoop; // 先告诉编译器有这种类型

struct Dispatcher
{
	// init -- 初始化 epoll，poll 或者 select 需要的数据块
	void* (*init)();
	// 添加
	int (*add)(struct Channel* channel,struct EventLoop* evLoop);
	// 删除
	int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
	// 修改
	int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
	// 事件检测
	int (*dispatch)(struct EventLoop* evLoop, int timeout); // 超时时常单位 s
	// 清除数据（关闭fd或者释放内存）
	int (*clear)(struct EventLoop* evLoop);
};




