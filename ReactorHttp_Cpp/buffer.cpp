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
	// 1.�ڴ湻�� - ����Ҫ����
	if (writeableSize() >= size)
	{
		return;
	}
	// 2.�ڴ���Ҫ�ϲ��Ź��� - ����Ҫ����
	// �ϲ���ʣ��Ŀ�д���ڴ� + �Ѷ��ڴ� >= size
	else if (m_readPos + writeableSize() >= size)
	{
		// �õ�δ�����ڴ��С
		int readable = readableSize();
		// �ƶ��ڴ�
		memcpy(m_data, m_data + m_readPos, readable);
		// ����λ��
		m_readPos = 0;
		m_writePos = readable;
	}
	// 3.�ڴ治���� - ����
	else
	{
		void* tmp = realloc(m_data, m_capacity + size);
		if (NULL == tmp)
		{
			return; // ʧ����
		}
		// ��������
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
	// ����
	extendRoom(size);
	// ��������
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
	// ��ʼ������Ԫ��
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
	else if (result <= writeable) // ȫд���� vec[0].iov_base ��
	{
		m_writePos += result;
	}
	else // ���ݹ��࣬ʣ�µ�д���� vec[1].iov_base ��
	{
		// readv �Ѿ��� buffer �����������ݣ����� writePos
		m_writePos = m_capacity;
		// ����� buffer �ڴ����չ
		appendString(tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char* Buffer::findCRLF()
{
	// strstr --> ���ַ�����ƥ�����ַ��������� \0 ������
	// char *strstr(const char *haystack, const char *needle);
	// memmem --> �����ݿ���ƥ�������ݿ飨��Ҫָ�����ݿ��С��
	//  void *memmem(const void *haystack, size_t haystacklen,
	//		const void* needle, size_t needlelen);
	char* ptr = (char*)memmem(m_data + m_readPos, readableSize(), "\r\n", 2);
	return ptr;
}

int Buffer::sendData(int socket)
{
	// �ж���������
	int readable = readableSize();
	if (readable > 0)
	{
		// ����� MSG_NOSIGNAL ��ʾ�������� linux �ں˵��ж��ź�
		// ����ź������ڶԷ��ر���ʽ����˿���ɵ�
		int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
		if (count > 0)
		{
			m_readPos += count;
			usleep(1); // ��Ϣһ΢�룬�ý��ն���ʱ�����
		}
		return count;
	}
	return 0;
}
