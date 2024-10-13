#pragma once

#include "buffer.h"
#include "httpResponse.h"

// ����ͷ��ֵ��
struct RequestHeader
{
	char* key;
	char* value;
};

// ��ǰ�Ľ���״̬
enum HttpRequestState
{
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone,
};

// ���� http ����ṹ��
struct HttpRequest
{
	char* method;
	char* url;
	char* version;
	struct RequestHeader* reqHeaders;
	int reqHeadersNum;
	enum HttpRequestState curState;
};

// ��ʼ��
struct HttpRequest* httpRequestInit();
// ����
void httpRequestReset(struct HttpRequest* req);
void httpRequestResetEX(struct HttpRequest* req);
// �ڴ��ͷ�
void httpRequestDestroy(struct HttpRequest* req);
// ��ȡ����״̬
enum HttpRequestState httpRequestState(struct HttpRequest* request);
// �������ͷ
void httpRequestAddHeader(struct HttpRequest* request, char* key, char* value);
// ���� key �õ�����ͷ�� value
char* httpRequestGetHeader(struct HttpRequest* request, char* key);
// ��������ͷ
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf);
// ��������ͷ
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf);
// ���� get ������ http ����
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf,
	struct HttpResponse* response, struct Buffer* sendBuf, int socket);
// ���� get ������ http ����Э��
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);
// �����ַ���
void decodeMsg(char* to, char* from);
// �õ��ļ�����
char* getFileType(const char* name);
// ����Ŀ¼
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);
// �����ļ�
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd);
