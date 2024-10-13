#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "buffer.h"
#include "httpRequest.h"
#include "httpResponse.h"

// ע�⣬������������Ʒ������ݵķ�ʽ���򿪺͹ر�ʱ���ַ��ͷ���
// ������ʱ����д�¼�ע�ᣬ��������һ�ֽ���Ⲣ��������
// ����ʱ���ǽ��ļ���д�뵽����Ķ��ڴ� sendBuf �У��������ڴ治���õ�����
// ��������ʱ��ֱ������дһ�㷢һ�㣬ֱ������������ϣ������ڴ治����
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
	// http Э��
	HttpRequest* m_request;
	HttpResponse* m_response;
};