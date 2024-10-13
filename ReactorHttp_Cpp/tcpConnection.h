#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "buffer.h"
#include "httpRequest.h"
#include "httpResponse.h"

// 注意，这个宏用来控制发送数据的方式，打开和关闭时两种发送方法
// 被定义时，将写事件注册，服务器下一轮将检测并发送数据
// 但此时，是将文件都写入到申请的堆内存 sendBuf 中，可能有内存不够用的问题
// 不被定义时，直接慢慢写一点发一点，直到发送数据完毕，不怕内存不够用
//#define MSG_SEND_AUTO

class TcpConnection
{
public:
	TcpConnection(int fd, EventLoop* evLoop);
	~TcpConnection();

	static int processRead(void* arg);
	static int processWrite(void* arg);
	static int destroy(void* arg);
private:
	std::string m_name;	
	EventLoop* m_evLoop;
	Channel* m_channel;
	Buffer* m_readBuf;
	Buffer* m_writeBuf;
	// http 协议
	HttpRequest* m_request;
	HttpResponse* m_response;
};