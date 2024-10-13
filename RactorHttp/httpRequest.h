#pragma once

#include "buffer.h"
#include "httpResponse.h"

// 请求头键值对
struct RequestHeader
{
	char* key;
	char* value;
};

// 当前的解析状态
enum HttpRequestState
{
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone,
};

// 定义 http 请求结构体
struct HttpRequest
{
	char* method;
	char* url;
	char* version;
	struct RequestHeader* reqHeaders;
	int reqHeadersNum;
	enum HttpRequestState curState;
};

// 初始化
struct HttpRequest* httpRequestInit();
// 重置
void httpRequestReset(struct HttpRequest* req);
void httpRequestResetEX(struct HttpRequest* req);
// 内存释放
void httpRequestDestroy(struct HttpRequest* req);
// 获取处理状态
enum HttpRequestState httpRequestState(struct HttpRequest* request);
// 添加请求头
void httpRequestAddHeader(struct HttpRequest* request, char* key, char* value);
// 根据 key 得到请求头的 value
char* httpRequestGetHeader(struct HttpRequest* request, char* key);
// 解析请求头
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf);
// 解析请求头
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf);
// 解析 get 方法的 http 请求
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf,
	struct HttpResponse* response, struct Buffer* sendBuf, int socket);
// 处理 get 方法的 http 请求协议
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);
// 解码字符串
void decodeMsg(char* to, char* from);
// 得到文件类型
char* getFileType(const char* name);
// 发送目录
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);
// 发送文件
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd);
