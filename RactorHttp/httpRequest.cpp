#include "httpRequest.h"
#include "tcpConnection.h"
#include "log.h"

#include <malloc.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>


#define HeaderSize 12

struct HttpRequest* httpRequestInit()
{
	struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
	httpRequestReset(request);
	request->reqHeaders = (struct RequestHeader*)malloc(sizeof(struct RequestHeader) * HeaderSize);
	return request;
}

void httpRequestReset(struct HttpRequest* req)
{
	req->curState = ParseReqLine;
	req->method = NULL;
	req->url = NULL;
	req->version = NULL;
	req->reqHeadersNum = 0;
}

void httpRequestResetEX(struct HttpRequest* req)
{
	free(req->method);
	free(req->url);
	free(req->version);
	if (NULL != req->reqHeaders)
	{
		for (int i = 0; i < req->reqHeadersNum; ++i)
		{
			free(req->reqHeaders[i].key);
			free(req->reqHeaders[i].value);
		}
		free(req->reqHeaders);
	}
	httpRequestReset(req);
}

void httpRequestDestroy(struct HttpRequest* req)
{
	if (req != NULL)
	{
		httpRequestResetEX(req);
		free(req);
	}
}

enum HttpRequestState httpRequestState(struct HttpRequest* request)
{
	return request->curState;
}

void httpRequestAddHeader(struct HttpRequest* request, char* key, char* value)
{
	request->reqHeaders[request->reqHeadersNum].key = key;
	request->reqHeaders[request->reqHeadersNum].value = value;
	request->reqHeadersNum++;
}

char* httpRequestGetHeader(struct HttpRequest* request, char* key)
{
	if (NULL != request)
	{
		for (int i = 0; i < request->reqHeadersNum; ++i)
		{
			if (0 == strncasecmp(request->reqHeaders[i].key, key, strlen(key)))
			{
				return request->reqHeaders[i].value;
			}
		}
	}
	return NULL;
}

char* splitRequestLine(char* start, char* end, char* sub, char** ptr)
// 注意第四个参数是指针的地址，是一个二级指针，由于要给函数体外的指针分配内存空间
// 而传一级指针则是值传递，函数体内的是形参，为外部指针的副本
// 如果为此指针分配内存，则此指针指向一块内存，外部指针还是没有分配内存，没有指向此块分配的内存
// 因此，不能为调用函数的实参指针分配内存,所以应该传指针的指针，即指向一个 char* 的指针
// 总体来说，如果我们想要在一个函数里面，对外部传入的指针分配一块内存，则需要传入此指针的地址
{
	char* space = end;
	if (sub != NULL)
	{
		space = (char*)memmem(start, end - start, sub, strlen(sub));
		assert(NULL != space);
	}
	int length = space - start;
	char* tmp = (char*)malloc(length + 1);
	strncpy(tmp, start, length);
	tmp[length] = '\0';
	*ptr = tmp; // 真正的函数体外部的指针，则指向刚刚分配的内存
	return space + 1;
}

// 这里不是用 sscanf 了，用的是指针和内存查找与字符串操作
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf)
{
	// 读出请求行
	char* end = bufferFindCRLF(readBuf);
	// 保存字符串起始地址
	char* start = readBuf->data + readBuf->readPos;
	// 请求行总长度
	int lineSize = end - start;
	// 保存字符串结束地址
	if (lineSize)
	{
		// get /xxx/xx.txt http/1.1
		// 相当于 &request->method 是一个传入传出参数
		start = splitRequestLine(start, end, " ", &request->method);
		start = splitRequestLine(start, end, " ", &request->url);
		splitRequestLine(start, end, NULL, &request->version);
#if 0
		// get /xxx/xx.txt http/1.1
		// 请求方式
		char* space = (char*)memmem(start, lineSize, " ", 1);
		assert(NULL != space);
		int methodSize = space - start;
		request->method = (char*)malloc(methodSize + 1);
		strncpy(request->method, start, methodSize);
		request->method[methodSize] = '\0';

		// 请求的静态资源
		start = space + 1;
		char* space = (char*)memmem(start, end - start, " ", 1);
		assert(NULL != space);
		int urlSize = space - start;
		request->url = (char*)malloc(urlSize + 1);
		strncpy(request->url, start, urlSize);
		request->url[urlSize] = '\0';

		// http 版本
		start = space + 1;
		//char* space = (char*)memmem(start, end - start, " ", 1);
		//assert(NULL != space);
		//int urlSize = space - start;
		request->version = (char*)malloc(end - start + 1);
		strncpy(request->version, start, end - start);
		request->version[end - start] = '\0';
#endif

		// 为解析请求头做准备
		readBuf->readPos += lineSize;
		readBuf->readPos += 2; // 跳过 \r\n

		// 修改状态
		request->curState = ParseReqHeaders;

		return true;
	}

	return false;
}

// 该函数处理了请求头中的一行
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf)
{
	char* end = bufferFindCRLF(readBuf);
	if (NULL != end)
	{
		char* start = readBuf->data + readBuf->readPos;
		int lineSize = end - start;
		// 基于 ： 搜索字符串
		char* middle = (char*)memmem(start, lineSize, ": ", 2);
		if (middle != NULL)
		{
			// 分配堆内存，并交给 key 指针
			char* key = (char*)malloc(middle - start + 1);
			strncpy(key, start, middle - start);
			key[middle - start] = '\0';
			// 分配堆内存，并交给 value 指针,注意有 ": "
			char* value = (char*)malloc(end - middle - 2 + 1);
			strncpy(value, middle + 2, end - middle - 2);
			value[end - middle - 2] = '\0';

			httpRequestAddHeader(request, key, value);

			// 移动读数据的位置
			readBuf->readPos += lineSize;
			readBuf->readPos += 2; // 跳过这一行的 \r\n
		}
		else
		{
			// 可能是找到了 http 的第三部分空行 \r\n， 请求头被解析完了
			readBuf->readPos += 2;
			// 修改解析状态
			// 忽略 post 请求，按照 get 请求处理
			request->curState = ParseReqDone;
		}
		return true;
	}
	return false;
}

bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf,
	struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
	bool flag = true;

	while (ParseReqDone != request->curState)
	{
		switch (request->curState)
		{
		case ParseReqLine:
			flag = parseHttpRequestLine(request, readBuf);
			break;
		case ParseReqHeaders:
			flag = parseHttpRequestHeader(request, readBuf);
			break;
		case ParseReqBody:
			break;
		default:
			break;
		}
		if (!flag) // 解析失败
		{
			return flag;
		}
		// 判断是否解析完毕了，如果完毕了，需要准备回复的数据
		if (ParseReqDone ==  request->curState)
		{
			// 1.根据解析出的原始数据，对客户端的请求作出处理
			processHttpRequest(request, response);
			// 2.组织响应数据并发送给客户端
			httpResponsePrepareMsg(response, sendBuf, socket);
		}
	}
	request->curState = ParseReqLine; // 状态还原，保证还能继续处理后面的请求
	return flag;
}

bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response)
{
	// 解析请求行
	// get /xxx/1.jpg http1.1
	// 注意其中的 /xxx/1.jpg 的 / 指的是客户端请求的那个目录，而不是服务器根目录

	if (0 != strcasecmp(request->method, "get")) // 我们这里只处理 get 请求
	{
		return -1;
	}
	// 处理一下字符串的转义字符
	decodeMsg(request->url, request->url);
	// 处理客户端请求的静态资源（目录或文件）
	char* file = NULL;
	if (0 == strcmp(request->url, "/"))
	{
		file = "./"; // 转换成相对路径
	}
	else
	{
		// 指向 path 指针下一个字符，提取 file，这个 file 可能是文件或目录
		file = request->url + 1; 
	}
	// 获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (-1 == ret)
	{
		// 文件不存在，先发送http头，再回复404（发送404.html）
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd);
		strcpy(response->fileName, "404.html");
		response->statusCode = NotFound;
		strcpy(response->statusMsg, "Not Found");
		// 添加响应头
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;
	}
	// 找到了
	strcpy(response->fileName, file);
	response->statusCode = OK;
	strcpy(response->statusMsg, "OK");
	// 判断文件类型
	if (1 == S_ISDIR(st.st_mode)) // 是目录类型 
	{
		// 把这个目录的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		//sendDir(file, cfd);
		// 添加响应头
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
		return 0;

	}
	else // 是文件类型
	{
		// 先发送http头，再把文件的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		//sendFile(file, cfd);
		// 添加响应头
		char tmp[12] = {0};
		sprintf(tmp,"%ld", st.st_size);
		httpResponseAddHeader(response, "Content-type", getFileType(file));
		httpResponseAddHeader(response, "Content-length",tmp);
		response->sendDataFunc = sendFile;
		return 0;
	}
	return false;
}

// 将字符转换为整形数
int hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2;
		}
		else
		{
			// 字符拷贝, 赋值
			*to = *from;
		}

	}
	*to = '\0';
}

char* getFileType(const char* name)
{
	// a.jpg a.mp4 a.html
	// 自右向左查找‘.’并返回指向此字符的指针，不存在则返回 NULL
	const char* dot = strrchr(name, '.');
	if (NULL == dot)
		return "text/plain; charset = utf-8"; // 纯文本
	if (0 == strcmp(dot, ".html") || 0 == strcmp(dot, ".htm"))
		return "text/html; charset = utf-8";
	if (0 == strcmp(dot, ".jpg") || 0 == strcmp(dot, ".jpeg"))
		return "image/jpeg";
	if (0 == strcmp(dot, ".gif"))
		return "image/gif";
	if (0 == strcmp(dot, ".png"))
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset = utf-8";
}

void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd)
{
	// 用 linux 提供的 scandir 函数扫描目录，并组装 html 发送给客户端，浏览器即可按格式显示
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort); // 返回 num 个文件
	for (int i = 0; i < num; ++i)
	{
		// 取出文件名，namelist是一个二级指针，指向一个指针数组 struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		// 得到文件或目录的属性
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name); // 拼接为正确文件/目录的路径名称
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode)) // 如果是目录，将其名称添加到 html 标签中
		{
			// 每行加上文件名和大小
		   // a标签, <a href="">name</a> ,设置跳转
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else // 如果是文件，herf 设置跳转时不加 /
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		//send(cfd, buf, strlen(buf), 0); // 直接发送数据，因为 TCP 流式传输协议发送端可以一点一点发，接收端一点一点收
		bufferAppendString(sendBuf,buf); // 不发了，改为往 buffer 里加数据
#ifndef MSG_SEND_AUTO
		bufferSendData(sendBuf, cfd); // 通过 buffer 发送数据，这里是写一点发一点
#endif 
		memset(buf, 0, sizeof(buf)); // 清空缓冲区
		free(namelist[i]); // 释放掉 scandir 为 namelist[i] 分配的内存
	}
	sprintf(buf, "</table></body></html>"); // 拼接 html 尾端
	//send(cfd, buf, strlen(buf), 0); // 发送 html 尾端
	bufferAppendString(sendBuf, buf); // 不直接发了，改为往 buffer 里加数据
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, cfd); // 通过 buffer 发送数据，这里是写一点发一点
#endif 
	free(namelist); // 释放掉 scandir 为 namelist 分配的内存
}

void sendFile(const char* fileName,struct Buffer* sendBuf, int cfd)
{
	// 打开文件，读一部分文件发一部分
	// 1.打开文件
	int fd = open(fileName, O_RDONLY);
	// 断言打开了文件，断言是严苛的，不允许程序发生错误，失败则抛出异常
	assert(fd > 0);
	// 发送数据
#if 1
	while (1) // 自己写发送文件的程序
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);
		if (len > 0)
		{
			//send(cfd, buf, len, 0);
			bufferAppendData(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
			bufferSendData(sendBuf, cfd); // 通过 buffer 发送数据，这里是写一点发一点
#endif 
			//usleep(10); // 休眠10ms，这非常重要，如果发送，则速度过快会导致收端出现意料之外的问题
		}
		else if (0 == len)
		{
			break;
		}
		else
		{
			close(fd);
			Error("read");
		}
	}
#else // 由于这里我们想要把所有数据都从 sendBuf 里发出，所以和 simpleHttp 不一样，不用下面的发送代码了
	// 使用 sendfile 系统自带函数，更快，因为去掉了用户态到内核态的拷贝
	off_t offset = 0; // 这是一个偏移量，由 sendfile 自动管理，可通过此偏移量和文件大小判断文件是否发完
	int size = lseek(fd, 0, SEEK_END); // 通过文件偏移量算出文件大小（文件指针移动到了尾部）
	lseek(fd, 0, SEEK_SET); // 用 lseek 函数将文件指针移动到头部
	while (offset < size)
	{
		int res = sendfile(cfd, fd, &offset, size - offset);
		//printf("res: %d\n", res);
		// 这时有时候 res 还是 -1，但是最后成功发完了
		// 这是由于写缓冲区发送到 cfd 太快，有时没来的及从 fd 写入
		if (-1 == res)
		{
			perror("sendfile");
			printf("errno:%d\n", errno);
		}
	}
#endif
	close(fd);
}