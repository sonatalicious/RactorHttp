#pragma once
#include <functional>

// ���庯��ָ��
//typedef	int(*handleFunc)(void* arg);
//using handleFunc = int(*)(void* ); // C++ 11 ������ using

// �����ļ��������Ķ�д�¼�
// ��Ϊǿ����ö��	
enum class FDEvent
{
	TimeOut = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};

// �ɵ��ö����װ���������ʲ�� 1������ָ�� 2���ɵ��ö��󣨿�������һ��ʹ�õĶ���
// ���յõ��˵�ַ������û�е��ã������������ "int(void*)" Ϊ����ǩ��
// std::function �����������洢�������ǩ��ƥ��Ŀɵ��ö���
// �ɵ��ö����װ����һ����ģ�壬�������ֽ� function
// ���潫ʹ�� function �� int(*)(void* )���з�װ
// std::function �� std::bind ��������Ŀ�����ʹ�ã�д�йػص���������ʱ������ʹ��
// Ҫע��,eventLoop �о��õ���
// std::bind ��һ������ģ�壬���ɵ��ö�������Ĳ�������һ�𣬲��� std::function ��װ������

class Channel
{
public:
	// �ÿɵ��ö����װ������װһ�� int(void*) ��Ȼ��������װ������һ�� handleFunc ����
	using handleFunc = std::function<int(void*)>; 
	Channel(int fd, FDEvent events, handleFunc readFunc,\
		handleFunc writeFunc, handleFunc destroyFunc, void* arg);
	// �ص�����
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
	// �޸� fd д�¼������ or ����⣩
	void writeEventEnable( bool flag);
	// �ж��Ƿ���Ҫ����ļ���������д�¼�
	bool isWriteEventEnable();
	// ȡ��˽�г�Ա��ֵ
	inline int getEvent() // inline ��ѹջ��ֱ���滻�����
	{
		return this->m_events;
	}
	inline int getSocket()
	{
		return this->m_fd;
	}
	inline const void* getArg()
	{
		return this->m_arg;
	}
private:
	// �ļ�������
	int m_fd;
	// �¼�
	int m_events;
	// �ص������Ĳ���
	void* m_arg;
};


