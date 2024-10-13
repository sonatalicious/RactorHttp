#pragma once

#include <string>

class Buffer
{
public:
	Buffer(int size);
	~Buffer();
	// 扩容
	void extendRoom(int size);
	// 得到剩余的可写的内存容量
	inline int writeableSize()
	{
		return m_capacity - m_writePos;
	}
	// 得到剩余的可读的内存容量
	inline int readableSize()
	{
		return m_writePos - m_readPos;
	}
	// 写内存 1.直接写 2.接受套接字数据
	int appendString(const char* data, int size);
	int appendString(const char* data);
	int appendString(const std::string data);
	int socketRead(int fd);
	// 根据\r\n取出一行，找到其在字符串中的位置，返回该位置
	char* findCRLF();
	// 发送数据
	int sendData(int socket);
	// 得到读数据起始位置
	inline char* data()
	{
		return m_data + m_readPos;
	}
	// 由于封装了，所以在外部修改 m_readPos 需要这个 API 函数
	inline int readPosIncrease(int count)
	{
		m_readPos += count;
		return m_readPos;
	}
private:
	// 指向内存的指针
	char* m_data;
	int m_capacity;
	int m_readPos = 0;
	int m_writePos = 0;
};

// 有三种初始化操作
// 1.像上面的 m_readPos = 0 一样定义时初始化
// 2.列表初始化 构造函数后面加一个：，像 :m_capacity(size) 一样初始化
// 3.构造函数内初始化
// 优先级顺序是 1,2,3
// 也就是说如果某个成员，在三个地方都作了初始化，则 3 生效，因为其会覆盖之前的操作
