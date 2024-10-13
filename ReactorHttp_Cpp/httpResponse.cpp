#include "httpResponse.h"
#include "tcpConnection.h"
#include "log.h"

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>

#pragma execution_character_set("utf-8") // 或 "gbk"
// 还是有中文乱码问题

HttpResponse::HttpResponse()
{
	m_statusCode = StatusCode::Unknown;
	m_headers.clear();
	m_fileName = std::string();
	sendDataFunc = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const std::string key, const std::string value)
{
	if (key.empty() || value.empty())
	{
		return;
	}
	m_headers.insert(std::make_pair(key, value));

}

void HttpResponse::prepareMsg(Buffer* sendBuf, int socket)
{
	// 状态行
	char tmp[1024] = { 0 };
	int code = (int)m_statusCode;
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", m_statusCode, m_info.at(code).data());
	sendBuf->appendString(tmp);
	// 响应头
	for (auto it = m_headers.begin(); it != m_headers.end(); ++it)
	{
		sprintf(tmp, "%s: %s\r\n",it->first.data(), it->second.data());
		sendBuf->appendString(tmp);
	}
	// 空行
	sendBuf->appendString("\r\n");
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(socket); // 通过 buffer 发送数据
#endif 
	// 回复的数据
	sendDataFunc(m_fileName, sendBuf, socket);
}
