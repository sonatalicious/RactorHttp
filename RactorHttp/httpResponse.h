#pragma once

#include "buffer.h"

// 定义状态码枚举
enum HttpStatusCode
{
	Unknown,
	OK = 200, // 成功
	MovedPermanently = 301, // 永久重定向
	MovedTemporarily = 302, // 临时重定向
	BadRequest = 400, // 错误请求
	NotFound = 404 // 资源找不到
};

// 定义相应的键值对结构体
struct ResponseHeader
{
	char key[32];
	char value[128];
};

// 定义一个函数指针，用来组织要回复给客户端的数据块
typedef void(*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);

// 定义 http 响应结构体
struct HttpResponse
{
	// 状态行：状态码，状态描述
	enum HttpStatusCode statusCode;
	char statusMsg[128]; 
	// 文件名称
	char fileName[256]; // 从 128 改成 256 了，不然文件名过大会有问题，早知道应该在堆上分配内存就好了
	// 响应头 - 键值对
	struct ResponseHeader* headers;
	int headerNum;
	responseBody sendDataFunc; // 是一个回调函数
};

// 初始化
struct HttpResponse* httpResponseInit();
// 销毁
void httpResponseDestroy(struct HttpResponse* response);
// 添加响应头
void httpResponseAddHeader(struct HttpResponse* response, char* key, char* value);
// 组织 http 响应数据
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket);
