#pragma once

#include "buffer.h"
#include <map>
#include <functional>

// 定义状态码枚举
enum class StatusCode
{
	Unknown,
	OK = 200, // 成功
	MovedPermanently = 301, // 永久重定向
	MovedTemporarily = 302, // 临时重定向
	BadRequest = 400, // 错误请求
	NotFound = 404 // 资源找不到
};

// 定义 http 响应结构体
struct HttpResponse
{
public:
	std::function<void(const std::string , Buffer* , int )> sendDataFunc; // 是一个可调用对象
	HttpResponse();
	~HttpResponse();
	// 添加响应头
	void addHeader(const std::string key, const std::string value);
	// 组织 http 响应数据
	void prepareMsg(Buffer* sendBuf, int socket);
	inline void setFileName(std::string name)
	{
		m_fileName = name;
	}
	inline void setStatusCode(StatusCode code)
	{
		m_statusCode = code;
	}

private:
	// 状态行：状态码，状态描述
	StatusCode m_statusCode;
	// 文件名称
	std::string m_fileName; 
	// 响应头 - 键值对
	std::map<std::string,std::string> m_headers;
	// 定义状态码和描述的关系
	const std::map<int, std::string> m_info = {
		{200,"OK"},
		{301,"MovedPermanently"},
		{302,"MovedTemporarily"},
		{400,"BadRequest"},
		{404,"NotFound"}
	};
};

