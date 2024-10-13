#pragma once

#include "buffer.h"
#include "httpResponse.h"

#include <map>

// 当前的解析状态
enum class ProcessState:char
{
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone,
};

// 定义 http 请求结构体
class HttpRequest
{
public:
	HttpRequest();
	~HttpRequest();

	// 重置
	void reset();
	// 添加请求头
	void addHeader(const std::string key,const std::string value);
	// 根据 key 得到请求头的 value
	std::string getHeader( const std::string key);
	// 解析请求行
	bool parseRequestLine( Buffer* readBuf);
	// 解析请求头中的一行
	bool parseRequestHeader( Buffer* readBuf);
	// 解析 get 方法的 http 请求
	bool parseHttpRequest(Buffer* readBuf,\
		HttpResponse* response, Buffer* sendBuf, int socket);
	// 处理 get 方法的 http 请求协议
	bool processHttpRequest( HttpResponse* response);
	// 解码字符串
	std:: string decodeMsg(std::string msg);
	// 得到文件类型
	const std::string getFileType(const std::string name);
	// 发送目录
	static void sendDir(std::string dirName, Buffer* sendBuf, int cfd);
	// 发送文件
	static void sendFile(std::string fileName, Buffer* sendBuf, int cfd);
	// 将字符转换为整形数
	inline void setMethod(std::string method)
	{
		m_method = method;
	}
	inline void setUrl(std::string url)
	{
		m_url = url;
	}
	inline void setVersion(std::string version)
	{
		m_version = version;
	}
	// 获取处理状态
	inline ProcessState getState()
	{
		return m_curState;
	}
	// 修改处理状态
	inline void setState(ProcessState state)
	{
		m_curState = state;
	}
private:

	// 这里用了可调用对象包装器
	char* splitRequestLine(char* start, char* end,\
		char* sub, std::function<void(std::string)> callback);
	int hexToDec(char c);

private:
	std::string m_method;
	std::string m_url;
	std::string m_version;
	std::map<std::string, std::string> m_reqHeaders;
	ProcessState m_curState;
};

