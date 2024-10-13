#include "httpResponse.h"
#include "tcpConnection.h"
#include "log.h"

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>

#define ResHeaderSize 16

struct HttpResponse* httpResponseInit()
{
	struct HttpResponse* response = (struct HttpResponse*)malloc(sizeof(struct HttpResponse));
	response->headerNum = 0;
	int size = sizeof(struct ResponseHeader) * ResHeaderSize;
	response->headers = (struct ResponseHeader*)malloc(size);
	response->statusCode = Unknown;
	// 初始化数组
	bzero(response->headers, size);
	bzero(response->statusMsg, sizeof(response->statusMsg));
	bzero(response->fileName, sizeof(response->fileName));
	// 函数指针
	response->sendDataFunc = NULL;
	return response;
}

void httpResponseDestroy(struct HttpResponse* response)
{
	if (NULL != response)
	{
		Debug("response Destroy:%s", response->fileName);
		free(response->headers);
		free(response);
	}
}

void httpResponseAddHeader(struct HttpResponse* response, char* key, char* value)
{
	if (NULL == response || NULL == key || NULL == value)
	{
		return;
	}
	if (response->headerNum >= ResHeaderSize)
	{
		return;
	}
	//Debug("addr:%p", response->headers);
	strcpy(response->headers[response->headerNum].key, key); // 这个 strcpy 真不能用，没有任何防护，以后用 strncpy
	strcpy(response->headers[response->headerNum].value, value); // 用 strcpy 的话，文件名称过大会直接破坏 response 结构体
	response->headerNum++;
}

void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
	// 状态行
	char tmp[1024] = { 0 };
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMsg);
	bufferAppendString(sendBuf, tmp);
	// 响应头
	for (int i = 0; i < response->headerNum; i++)
	{
		sprintf(tmp,"%s: %s\r\n", response->headers[i].key, response->headers[i].value);
		bufferAppendString(sendBuf, tmp);
	}
	// 空行
	bufferAppendString(sendBuf, "\r\n");
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, socket); // 通过 buffer 发送数据
#endif 
	// 回复的数据
	response->sendDataFunc(response->fileName, sendBuf, socket);
}
