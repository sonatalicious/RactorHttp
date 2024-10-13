#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "buffer.h"
#include "httpRequest.h"
#include "httpResponse.h"

// 注意，这个宏用来控制发送数据的方式，打开和关闭时两种发送方法
// 被定义时，将读事件注册，服务器下一轮将检测并发送数据
// 但此时，是将文件都写入到申请的堆内存 sendBuf 中，可能有内存不够用的问题
// 不被定义时，直接慢慢写一点发一点，直到发送数据完毕，不怕内存不够用
#define MSG_SEND_AUTO

struct TcpConnection
{
	struct EventLoop* evLoop;
	struct Channel* channel;
	struct Buffer* readBuf;
	struct Buffer* writeBuf;
	char name[32];
	// http 协议
	struct HttpRequest* request;
	struct HttpResponse* response;
};

// 初始化
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop);
// 释放资源
int tcpConnectionDestroy(void* arg);
