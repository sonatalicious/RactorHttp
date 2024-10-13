#include "buffer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>

Buffer::Buffer(int size):m_capacity(size)
{
	m_data = (char*)malloc(size);
	bzero(m_data, size);
}

Buffer::~Buffer()
{
	if (m_data != nullptr)
	{
		free(m_data);
	}
}

void Buffer::extendRoom(int size)
{
	// 1.内存够用 - 不需要扩容
	if (writeableSize() >= size)
	{
		return;
	}
	// 2.内存需要合并才够用 - 不需要扩容
	// 合并：剩余的可写的内存 + 已读内存 >= size
	else if (m_readPos + writeableSize() >= size)
	{
		// 得到未读的内存大小
		int readable = readableSize();
		// 移动内存
		memcpy(m_data, m_data + m_readPos, readable);
		// 更新位置
		m_readPos = 0;
		m_writePos = readable;
	}
	// 3.内存不够用 - 扩容
	else
	{
		void* tmp = realloc(m_data, m_capacity + size);
		if (NULL == tmp)
		{
			return; // 失败了
		}
		// 更新数据
		memset((char*)tmp + m_capacity, 0, size);
		m_data = static_cast<char*>(tmp);
		m_capacity += size;
	}
}
int Buffer::appendString(const char* data, int size)
{
	if (NULL == data || size <= 0)
	{
		return -1;
	}
	// 扩容
	extendRoom(size);
	// 拷贝数据
	memcpy(m_data + m_writePos, data, size);
	m_writePos += size;
	return 0;
}

int Buffer::appendString(const char* data)
{
	int size = strlen(data);
	int ret = appendString(data, size);
	return ret;
}
int Buffer::appendString(const std::string data)
{
	int ret = appendString(data.data());
	return ret;
}
int Buffer::socketRead(int fd)
{
	// read/recv/readv
	struct iovec vec[2];
	// 初始化数组元素
	int writeable = writeableSize();
	vec[0].iov_base = m_data + m_writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960); // 40k
	vec[1].iov_base = tmpbuf;
	vec[1].iov_len = 40960;
	int result = readv(fd, vec, 2);
	if (-1 == result)
	{
		return -1;
	}
	else if (result <= writeable) // 全写到了 vec[0].iov_base 中
	{
		m_writePos += result;
	}
	else // 数据过多，剩下的写到了 vec[1].iov_base 中
	{
		// readv 已经让 buffer 接收满了数据，更新 writePos
		m_writePos = m_capacity;
		// 会进行 buffer 内存的扩展
		appendString(tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char* Buffer::findCRLF()
{
	// strstr --> 大字符串中匹配子字符串（遇到 \0 结束）
	// char *strstr(const char *haystack, const char *needle);
	// memmem --> 大数据块中匹配子数据块（需要指定数据块大小）
	//  void *memmem(const void *haystack, size_t haystacklen,
	//		const void* needle, size_t needlelen);
	char* ptr = (char*)memmem(m_data + m_readPos, readableSize(), "\r\n", 2);
	return ptr;
}

int Buffer::sendData(int socket)
{
	// 判断有无数据
	int readable = readableSize();
	if (readable > 0)
	{
		// 传入的 MSG_NOSIGNAL 表示，不接收 linux 内核的中断信号
		// 这个信号是由于对方关闭流式传输端口造成的
		int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
		if (count > 0)
		{
			m_readPos += count;
			usleep(1); // 休息一微秒，让接收端有时间接收
		}
		return count;
	}
	return 0;
}
