#pragma once

#include "buffer.h"
#include <map>
#include <functional>

// ����״̬��ö��
enum class StatusCode
{
	Unknown,
	OK = 200, // �ɹ�
	MovedPermanently = 301, // �����ض���
	MovedTemporarily = 302, // ��ʱ�ض���
	BadRequest = 400, // ��������
	NotFound = 404 // ��Դ�Ҳ���
};

// ���� http ��Ӧ�ṹ��
struct HttpResponse
{
public:
	std::function<void(const std::string , Buffer* , int )> sendDataFunc; // ��һ���ɵ��ö���
	HttpResponse();
	~HttpResponse();
	// �����Ӧͷ
	void addHeader(const std::string key, const std::string value);
	// ��֯ http ��Ӧ����
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
	// ״̬�У�״̬�룬״̬����
	StatusCode m_statusCode;
	// �ļ�����
	std::string m_fileName; 
	// ��Ӧͷ - ��ֵ��
	std::map<std::string,std::string> m_headers;
	// ����״̬��������Ĺ�ϵ
	const std::map<int, std::string> m_info = {
		{200,"OK"},
		{301,"MovedPermanently"},
		{302,"MovedTemporarily"},
		{400,"BadRequest"},
		{404,"NotFound"}
	};
};

