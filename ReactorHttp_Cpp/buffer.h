#pragma once

#include <string>

class Buffer
{
public:
	Buffer(int size);
	~Buffer();
	// ����
	void extendRoom(int size);
	// �õ�ʣ��Ŀ�д���ڴ�����
	inline int writeableSize()
	{
		return m_capacity - m_writePos;
	}
	// �õ�ʣ��Ŀɶ����ڴ�����
	inline int readableSize()
	{
		return m_writePos - m_readPos;
	}
	// д�ڴ� 1.ֱ��д 2.�����׽�������
	int appendString(const char* data, int size);
	int appendString(const char* data);
	int appendString(const std::string data);
	int socketRead(int fd);
	// ����\r\nȡ��һ�У��ҵ������ַ����е�λ�ã����ظ�λ��
	char* findCRLF();
	// ��������
	int sendData(int socket);
	// �õ���������ʼλ��
	inline char* data()
	{
		return m_data + m_readPos;
	}
	// ���ڷ�װ�ˣ��������ⲿ�޸� m_readPos ��Ҫ��� API ����
	inline int readPosIncrease(int count)
	{
		m_readPos += count;
		return m_readPos;
	}
private:
	// ָ���ڴ��ָ��
	char* m_data;
	int m_capacity;
	int m_readPos = 0;
	int m_writePos = 0;
};

// �����ֳ�ʼ������
// 1.������� m_readPos = 0 һ������ʱ��ʼ��
// 2.�б��ʼ�� ���캯�������һ�������� :m_capacity(size) һ����ʼ��
// 3.���캯���ڳ�ʼ��
// ���ȼ�˳���� 1,2,3
// Ҳ����˵���ĳ����Ա���������ط������˳�ʼ������ 3 ��Ч����Ϊ��Ḳ��֮ǰ�Ĳ���
