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
	// 1.内存够用 - 不需要扩容
	if (bufferWriteableSize(buffer) >= size)
	{
		return;
	}
	// 2.内存需要合并才够用 - 不需要扩容
	// 合并：剩余的可写的内存 + 已读内存 >= size
	else if (buffer->readPos + bufferWriteableSize(buffer) >= size)
	{
		// 得到未读的内存大小
		int readable = bufferReadableSize(buffer);
		// 移动内存
		memcpy(buffer->data, buffer->data + buffer->readPos, readable);
		// 更新位置
		buffer->readPos = 0;
		buffer->writePos = readable;
	}
	// 3.内存不够用 - 扩容
	else
	{
		void* tmp = realloc(buffer->data, buffer->capacity + size);
		if (NULL == tmp)
		{
			return; // 失败了
		}
		// 更新数据
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
	// 扩容
	bufferExtendRoom(buffer, size);
	// 拷贝数据
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
	// 初始化数组元素
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
	else if (result <= writeable) // 全写到了 vec[0].iov_base 中
	{
		buffer->writePos += result;
	}
	else // 数据过多，剩下的写到了 vec[1].iov_base 中
	{
		// readv 已经让 buffer 接收满了数据，更新 writePos
		buffer->writePos = buffer->capacity;
		// bufferAppendData 会进行 buffer 内存的扩展
		bufferAppendData(buffer, tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char* bufferFindCRLF(struct Buffer* buffer)
{
	// strstr --> 大字符串中匹配子字符串（遇到 \0 结束）
	// char *strstr(const char *haystack, const char *needle);
	// memmem --> 大数据块中匹配子数据块（需要指定数据块大小）
	//  void *memmem(const void *haystack, size_t haystacklen,
	//		const void* needle, size_t needlelen);
	char* ptr =(char*) memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
	return ptr;
}

int bufferSendData(struct Buffer* buffer, int socket)
{
	// 判断有无数据
	int readable = bufferReadableSize(buffer);
	if (readable > 0)
	{
		// 传入的 MSG_NOSIGNAL 表示，不接收 linux 内核的中断信号
		// 这个信号是由于对方关闭流式传输端口造成的
		int count = send(socket, buffer->data + buffer->readPos, readable, MSG_NOSIGNAL);
		if (count > 0)
		{
			buffer->readPos += count;
			usleep(1); // 休息一微秒，让接收端有时间接收
		}
		return count;
	}
	return 0;
}
