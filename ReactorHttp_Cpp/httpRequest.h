#pragma once

#include "buffer.h"
#include "httpResponse.h"

#include <map>

// ��ǰ�Ľ���״̬
enum class ProcessState:char
{
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone,
};

// ���� http ����ṹ��
class HttpRequest
{
public:
	HttpRequest();
	~HttpRequest();

	// ����
	void reset();
	// �������ͷ
	void addHeader(const std::string key,const std::string value);
	// ���� key �õ�����ͷ�� value
	std::string getHeader( const std::string key);
	// ����������
	bool parseRequestLine( Buffer* readBuf);
	// ��������ͷ�е�һ��
	bool parseRequestHeader( Buffer* readBuf);
	// ���� get ������ http ����
	bool parseHttpRequest(Buffer* readBuf,\
		HttpResponse* response, Buffer* sendBuf, int socket);
	// ���� get ������ http ����Э��
	bool processHttpRequest( HttpResponse* response);
	// �����ַ���
	std:: string decodeMsg(std::string msg);
	// �õ��ļ�����
	const std::string getFileType(const std::string name);
	// ����Ŀ¼
	static void sendDir(std::string dirName, Buffer* sendBuf, int cfd);
	// �����ļ�
	static void sendFile(std::string fileName, Buffer* sendBuf, int cfd);
	// ���ַ�ת��Ϊ������
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
	// ��ȡ����״̬
	inline ProcessState getState()
	{
		return m_curState;
	}
	// �޸Ĵ���״̬
	inline void setState(ProcessState state)
	{
		m_curState = state;
	}
private:

	// �������˿ɵ��ö����װ��
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

