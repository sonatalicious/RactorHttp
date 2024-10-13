#include "buffer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>

struct Buffer* bufferInit(int size)
{
	struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
	if (NULL != buffer)
	{
		buffer->data = (char*)malloc(size);
		buffer->capacity = size;
		buffer->writePos = buffer->readPos = 0;
		memset(buffer->data, 0, size);
	}
	return buffer;
}

void bufferDestroy(struct Buffer* buf)
{
	if (NULL != buf)
	{
		if (NULL != buf->data)
		{
			free(buf->data);
		}
		free(buf);
	}
}

void bufferExtendRoom(struct Buffer* buffer, int size)
{
	// 1.�ڴ湻�� - ����Ҫ����
	if (bufferWriteableSize(buffer) >= size)
	{
		return;
	}
	// 2.�ڴ���Ҫ�ϲ��Ź��� - ����Ҫ����
	// �ϲ���ʣ��Ŀ�д���ڴ� + �Ѷ��ڴ� >= size
	else if (buffer->readPos + bufferWriteableSize(buffer) >= size)
	{
		// �õ�δ�����ڴ��С
		int readable = bufferReadableSize(buffer);
		// �ƶ��ڴ�
		memcpy(buffer->data, buffer->data + buffer->readPos, readable);
		// ����λ��
		buffer->readPos = 0;
		buffer->writePos = readable;
	}
	// 3.�ڴ治���� - ����
	else
	{
		void* tmp = realloc(buffer->data, buffer->capacity + size);
		if (NULL == tmp)
		{
			return; // ʧ����
		}
		// ��������
		memset(tmp + buffer->capacity, 0, size);
		buffer->data = (char*)tmp;
		buffer->capacity += size;
	}
}

int bufferWriteableSize(struct Buffer* buffer)
{
	return buffer->capacity - buffer->writePos;
}

int bufferReadableSize(struct Buffer* buffer)
{

	return buffer->writePos - buffer->readPos;
}

int bufferAppendData(struct Buffer* buffer, const char* data, int size)
{
	if (NULL == buffer || NULL == data || size <= 0)
	{
		return -1;
	}
	// ����
	bufferExtendRoom(buffer, size);
	// ��������
	memcpy(buffer->data + buffer->writePos, data, size);
	buffer->writePos += size;
	return 0;
}

int bufferAppendString(struct Buffer* buffer, const char* data)
{
	int size = strlen(data);
	int ret = bufferAppendData(buffer, data, size);
	return ret;
}

int bufferSocketRead(struct Buffer* buffer, int fd)
{
	// read/recv/readv
	struct iovec vec[2];
	// ��ʼ������Ԫ��
	int writeable = bufferWriteableSize(buffer);
	vec[0].iov_base = buffer->data + buffer->writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960); // 40k
	vec[1].iov_base = tmpbuf;
	vec[1].iov_len = 40960;
	int result = readv(fd, vec, 2);
	if(-1 == result)
	{
		return -1;
	}
	else if (result <= writeable) // ȫд���� vec[0].iov_base ��
	{
		buffer->writePos += result;
	}
	else // ���ݹ��࣬ʣ�µ�д���� vec[1].iov_base ��
	{
		// readv �Ѿ��� buffer �����������ݣ����� writePos
		buffer->writePos = buffer->capacity;
		// bufferAppendData ����� buffer �ڴ����չ
		bufferAppendData(buffer, tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char* bufferFindCRLF(struct Buffer* buffer)
{
	// strstr --> ���ַ�����ƥ�����ַ��������� \0 ������
	// char *strstr(const char *haystack, const char *needle);
	// memmem --> �����ݿ���ƥ�������ݿ飨��Ҫָ�����ݿ��С��
	//  void *memmem(const void *haystack, size_t haystacklen,
	//		const void* needle, size_t needlelen);
	char* ptr =(char*) memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
	return ptr;
}

int bufferSendData(struct Buffer* buffer, int socket)
{
	// �ж���������
	int readable = bufferReadableSize(buffer);
	if (readable > 0)
	{
		// ����� MSG_NOSIGNAL ��ʾ�������� linux �ں˵��ж��ź�
		// ����ź������ڶԷ��ر���ʽ����˿���ɵ�
		int count = send(socket, buffer->data + buffer->readPos, readable, MSG_NOSIGNAL);
		if (count > 0)
		{
			buffer->readPos += count;
			usleep(1); // ��Ϣһ΢�룬�ý��ն���ʱ�����
		}
		return count;
	}
	return 0;
}
