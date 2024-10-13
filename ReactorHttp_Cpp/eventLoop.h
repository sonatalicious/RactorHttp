#pragma once

#include "dispatcher.h"
#include "channel.h"

#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <string>

// ����ýڵ��е� channel �ķ�ʽ
enum class ElemType:char{ADD, DELETE, MODIFY};
// ����������еĽڵ�
struct ChannelElement
{
	ElemType type; // ��δ���ýڵ��е� channel
	Channel* channel;
};

class Dispatcher; // �ȸ��߱���������������

class EventLoop
{
public:
	EventLoop();
	EventLoop(const std::string threadName);
	~EventLoop();

	// ������Ӧ��ģ��
	int run();
	// ��������� fd
	int eventActive(int fd, int event);
	// ��������������
	int addTask(Channel* channel, ElemType type);
	// ������������е�����
	int processTaskQ();
	// ���� dispatcher �еĽڵ�(������������еĽڵ㣩����ɾ�ļ��� fd
	int add(Channel* channel);
	int remove(Channel* channel);
	int modify(Channel* channel);
	// �ͷ� channel
	int freeChannel(Channel* channel);
	// �����߳� id
	inline std::thread::id getThreadID()
	{
		return m_threadID;
	}
	// channel.h �� using handleFunc = int(*)(void* );
	// ע�� handleFunc �Ǻ���ָ�룬ֻ��ָ����ͨ���������� C ���������ྲ̬����
	// ��Ϊ���Ա�����ڳ�ʼ��ǰ���ܴ��ڣ����Բ���ָ����ͨ���Ա����
	static int readLocalMessage(void* arg);
	int readMessage();
private:
	void taskWakeup();

private:
	bool m_isQuit;
	// ��ָ��ָ�������ʵ�� epoll��poll��Select
	Dispatcher* m_dispatcher;
	// ������� taskQueue(���̺߳ʹ����̹߳��ã�����װ��ν��У�
	std::queue< ChannelElement* > m_taskQ;
	// channel map
	std::map<int, Channel*> m_channelMap;
	// �߳�id��name�������� mutex(��Ҫ�����������)
	std::thread::id m_threadID;
	std::string m_threadName;
	std::mutex m_mutex;
	// �洢����ͨ�ŵ� fd��ͨ�� socketpair ��ʼ��
	int m_socketPair[2];
};
