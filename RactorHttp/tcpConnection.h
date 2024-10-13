#pragma once

#include "eventLoop.h"
#include "channel.h"
#include "buffer.h"
#include "httpRequest.h"
#include "httpResponse.h"

// ע�⣬������������Ʒ������ݵķ�ʽ���򿪺͹ر�ʱ���ַ��ͷ���
// ������ʱ�������¼�ע�ᣬ��������һ�ֽ���Ⲣ��������
// ����ʱ���ǽ��ļ���д�뵽����Ķ��ڴ� sendBuf �У��������ڴ治���õ�����
// ��������ʱ��ֱ������дһ�㷢һ�㣬ֱ������������ϣ������ڴ治����
#define MSG_SEND_AUTO

struct TcpConnection
{
	struct EventLoop* evLoop;
	struct Channel* channel;
	struct Buffer* readBuf;
	struct Buffer* writeBuf;
	char name[32];
	// http Э��
	struct HttpRequest* request;
	struct HttpResponse* response;
};

// ��ʼ��
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop);
// �ͷ���Դ
int tcpConnectionDestroy(void* arg);
