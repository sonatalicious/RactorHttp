#pragma once

#include "buffer.h"

// ����״̬��ö��
enum HttpStatusCode
{
	Unknown,
	OK = 200, // �ɹ�
	MovedPermanently = 301, // �����ض���
	MovedTemporarily = 302, // ��ʱ�ض���
	BadRequest = 400, // ��������
	NotFound = 404 // ��Դ�Ҳ���
};

// ������Ӧ�ļ�ֵ�Խṹ��
struct ResponseHeader
{
	char key[32];
	char value[128];
};

// ����һ������ָ�룬������֯Ҫ�ظ����ͻ��˵����ݿ�
typedef void(*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);

// ���� http ��Ӧ�ṹ��
struct HttpResponse
{
	// ״̬�У�״̬�룬״̬����
	enum HttpStatusCode statusCode;
	char statusMsg[128]; 
	// �ļ�����
	char fileName[256]; // �� 128 �ĳ� 256 �ˣ���Ȼ�ļ�������������⣬��֪��Ӧ���ڶ��Ϸ����ڴ�ͺ���
	// ��Ӧͷ - ��ֵ��
	struct ResponseHeader* headers;
	int headerNum;
	responseBody sendDataFunc; // ��һ���ص�����
};

// ��ʼ��
struct HttpResponse* httpResponseInit();
// ����
void httpResponseDestroy(struct HttpResponse* response);
// �����Ӧͷ
void httpResponseAddHeader(struct HttpResponse* response, char* key, char* value);
// ��֯ http ��Ӧ����
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket);
