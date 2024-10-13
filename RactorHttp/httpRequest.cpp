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
// ע����ĸ�������ָ��ĵ�ַ����һ������ָ�룬����Ҫ�����������ָ������ڴ�ռ�
// ����һ��ָ������ֵ���ݣ��������ڵ����βΣ�Ϊ�ⲿָ��ĸ���
// ���Ϊ��ָ������ڴ棬���ָ��ָ��һ���ڴ棬�ⲿָ�뻹��û�з����ڴ棬û��ָ��˿������ڴ�
// ��ˣ�����Ϊ���ú�����ʵ��ָ������ڴ�,����Ӧ�ô�ָ���ָ�룬��ָ��һ�� char* ��ָ��
// ������˵�����������Ҫ��һ���������棬���ⲿ�����ָ�����һ���ڴ棬����Ҫ�����ָ��ĵ�ַ
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
	*ptr = tmp; // �����ĺ������ⲿ��ָ�룬��ָ��ոշ�����ڴ�
	return space + 1;
}

// ���ﲻ���� sscanf �ˣ��õ���ָ����ڴ�������ַ�������
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf)
{
	// ����������
	char* end = bufferFindCRLF(readBuf);
	// �����ַ�����ʼ��ַ
	char* start = readBuf->data + readBuf->readPos;
	// �������ܳ���
	int lineSize = end - start;
	// �����ַ���������ַ
	if (lineSize)
	{
		// get /xxx/xx.txt http/1.1
		// �൱�� &request->method ��һ�����봫������
		start = splitRequestLine(start, end, " ", &request->method);
		start = splitRequestLine(start, end, " ", &request->url);
		splitRequestLine(start, end, NULL, &request->version);
#if 0
		// get /xxx/xx.txt http/1.1
		// ����ʽ
		char* space = (char*)memmem(start, lineSize, " ", 1);
		assert(NULL != space);
		int methodSize = space - start;
		request->method = (char*)malloc(methodSize + 1);
		strncpy(request->method, start, methodSize);
		request->method[methodSize] = '\0';

		// ����ľ�̬��Դ
		start = space + 1;
		char* space = (char*)memmem(start, end - start, " ", 1);
		assert(NULL != space);
		int urlSize = space - start;
		request->url = (char*)malloc(urlSize + 1);
		strncpy(request->url, start, urlSize);
		request->url[urlSize] = '\0';

		// http �汾
		start = space + 1;
		//char* space = (char*)memmem(start, end - start, " ", 1);
		//assert(NULL != space);
		//int urlSize = space - start;
		request->version = (char*)malloc(end - start + 1);
		strncpy(request->version, start, end - start);
		request->version[end - start] = '\0';
#endif

		// Ϊ��������ͷ��׼��
		readBuf->readPos += lineSize;
		readBuf->readPos += 2; // ���� \r\n

		// �޸�״̬
		request->curState = ParseReqHeaders;

		return true;
	}

	return false;
}

// �ú�������������ͷ�е�һ��
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf)
{
	char* end = bufferFindCRLF(readBuf);
	if (NULL != end)
	{
		char* start = readBuf->data + readBuf->readPos;
		int lineSize = end - start;
		// ���� �� �����ַ���
		char* middle = (char*)memmem(start, lineSize, ": ", 2);
		if (middle != NULL)
		{
			// ������ڴ棬������ key ָ��
			char* key = (char*)malloc(middle - start + 1);
			strncpy(key, start, middle - start);
			key[middle - start] = '\0';
			// ������ڴ棬������ value ָ��,ע���� ": "
			char* value = (char*)malloc(end - middle - 2 + 1);
			strncpy(value, middle + 2, end - middle - 2);
			value[end - middle - 2] = '\0';

			httpRequestAddHeader(request, key, value);

			// �ƶ������ݵ�λ��
			readBuf->readPos += lineSize;
			readBuf->readPos += 2; // ������һ�е� \r\n
		}
		else
		{
			// �������ҵ��� http �ĵ������ֿ��� \r\n�� ����ͷ����������
			readBuf->readPos += 2;
			// �޸Ľ���״̬
			// ���� post ���󣬰��� get ������
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
		if (!flag) // ����ʧ��
		{
			return flag;
		}
		// �ж��Ƿ��������ˣ��������ˣ���Ҫ׼���ظ�������
		if (ParseReqDone ==  request->curState)
		{
			// 1.���ݽ�������ԭʼ���ݣ��Կͻ��˵�������������
			processHttpRequest(request, response);
			// 2.��֯��Ӧ���ݲ����͸��ͻ���
			httpResponsePrepareMsg(response, sendBuf, socket);
		}
	}
	request->curState = ParseReqLine; // ״̬��ԭ����֤���ܼ���������������
	return flag;
}

bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response)
{
	// ����������
	// get /xxx/1.jpg http1.1
	// ע�����е� /xxx/1.jpg �� / ָ���ǿͻ���������Ǹ�Ŀ¼�������Ƿ�������Ŀ¼

	if (0 != strcasecmp(request->method, "get")) // ��������ֻ���� get ����
	{
		return -1;
	}
	// ����һ���ַ�����ת���ַ�
	decodeMsg(request->url, request->url);
	// ����ͻ�������ľ�̬��Դ��Ŀ¼���ļ���
	char* file = NULL;
	if (0 == strcmp(request->url, "/"))
	{
		file = "./"; // ת�������·��
	}
	else
	{
		// ָ�� path ָ����һ���ַ�����ȡ file����� file �������ļ���Ŀ¼
		file = request->url + 1; 
	}
	// ��ȡ�ļ�����
	struct stat st;
	int ret = stat(file, &st);
	if (-1 == ret)
	{
		// �ļ������ڣ��ȷ���httpͷ���ٻظ�404������404.html��
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd);
		strcpy(response->fileName, "404.html");
		response->statusCode = NotFound;
		strcpy(response->statusMsg, "Not Found");
		// �����Ӧͷ
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;
	}
	// �ҵ���
	strcpy(response->fileName, file);
	response->statusCode = OK;
	strcpy(response->statusMsg, "OK");
	// �ж��ļ�����
	if (1 == S_ISDIR(st.st_mode)) // ��Ŀ¼���� 
	{
		// �����Ŀ¼�����ݷ��͸��ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		//sendDir(file, cfd);
		// �����Ӧͷ
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
		return 0;

	}
	else // ���ļ�����
	{
		// �ȷ���httpͷ���ٰ��ļ������ݷ��͸��ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		//sendFile(file, cfd);
		// �����Ӧͷ
		char tmp[12] = {0};
		sprintf(tmp,"%ld", st.st_size);
		httpResponseAddHeader(response, "Content-type", getFileType(file));
		httpResponseAddHeader(response, "Content-length",tmp);
		response->sendDataFunc = sendFile;
		return 0;
	}
	return false;
}

// ���ַ�ת��Ϊ������
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

// to �洢����֮�������, ��������, from�����������, �������
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		// isxdigit -> �ж��ַ��ǲ���16���Ƹ�ʽ, ȡֵ�� 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// ��16���Ƶ��� -> ʮ���� �������ֵ��ֵ�����ַ� int -> char
			// B2 == 178
			// ��3���ַ�, �����һ���ַ�, ����ַ�����ԭʼ����
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

			// ���� from[1] �� from[2] ����ڵ�ǰѭ�����Ѿ��������
			from += 2;
		}
		else
		{
			// �ַ�����, ��ֵ
			*to = *from;
		}

	}
	*to = '\0';
}

char* getFileType(const char* name)
{
	// a.jpg a.mp4 a.html
	// ����������ҡ�.��������ָ����ַ���ָ�룬�������򷵻� NULL
	const char* dot = strrchr(name, '.');
	if (NULL == dot)
		return "text/plain; charset = utf-8"; // ���ı�
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
	// �� linux �ṩ�� scandir ����ɨ��Ŀ¼������װ html ���͸��ͻ��ˣ���������ɰ���ʽ��ʾ
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort); // ���� num ���ļ�
	for (int i = 0; i < num; ++i)
	{
		// ȡ���ļ�����namelist��һ������ָ�룬ָ��һ��ָ������ struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		// �õ��ļ���Ŀ¼������
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name); // ƴ��Ϊ��ȷ�ļ�/Ŀ¼��·������
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode)) // �����Ŀ¼������������ӵ� html ��ǩ��
		{
			// ÿ�м����ļ����ʹ�С
		   // a��ǩ, <a href="">name</a> ,������ת
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else // ������ļ���herf ������תʱ���� /
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		//send(cfd, buf, strlen(buf), 0); // ֱ�ӷ������ݣ���Ϊ TCP ��ʽ����Э�鷢�Ͷ˿���һ��һ�㷢�����ն�һ��һ����
		bufferAppendString(sendBuf,buf); // �����ˣ���Ϊ�� buffer �������
#ifndef MSG_SEND_AUTO
		bufferSendData(sendBuf, cfd); // ͨ�� buffer �������ݣ�������дһ�㷢һ��
#endif 
		memset(buf, 0, sizeof(buf)); // ��ջ�����
		free(namelist[i]); // �ͷŵ� scandir Ϊ namelist[i] ������ڴ�
	}
	sprintf(buf, "</table></body></html>"); // ƴ�� html β��
	//send(cfd, buf, strlen(buf), 0); // ���� html β��
	bufferAppendString(sendBuf, buf); // ��ֱ�ӷ��ˣ���Ϊ�� buffer �������
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, cfd); // ͨ�� buffer �������ݣ�������дһ�㷢һ��
#endif 
	free(namelist); // �ͷŵ� scandir Ϊ namelist ������ڴ�
}

void sendFile(const char* fileName,struct Buffer* sendBuf, int cfd)
{
	// ���ļ�����һ�����ļ���һ����
	// 1.���ļ�
	int fd = open(fileName, O_RDONLY);
	// ���Դ����ļ����������Ͽ��ģ����������������ʧ�����׳��쳣
	assert(fd > 0);
	// ��������
#if 1
	while (1) // �Լ�д�����ļ��ĳ���
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);
		if (len > 0)
		{
			//send(cfd, buf, len, 0);
			bufferAppendData(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
			bufferSendData(sendBuf, cfd); // ͨ�� buffer �������ݣ�������дһ�㷢һ��
#endif 
			//usleep(10); // ����10ms����ǳ���Ҫ��������ͣ����ٶȹ���ᵼ���ն˳�������֮�������
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
#else // ��������������Ҫ���������ݶ��� sendBuf �﷢�������Ժ� simpleHttp ��һ������������ķ��ʹ�����
	// ʹ�� sendfile ϵͳ�Դ����������죬��Ϊȥ�����û�̬���ں�̬�Ŀ���
	off_t offset = 0; // ����һ��ƫ�������� sendfile �Զ�������ͨ����ƫ�������ļ���С�ж��ļ��Ƿ���
	int size = lseek(fd, 0, SEEK_END); // ͨ���ļ�ƫ��������ļ���С���ļ�ָ���ƶ�����β����
	lseek(fd, 0, SEEK_SET); // �� lseek �������ļ�ָ���ƶ���ͷ��
	while (offset < size)
	{
		int res = sendfile(cfd, fd, &offset, size - offset);
		//printf("res: %d\n", res);
		// ��ʱ��ʱ�� res ���� -1���������ɹ�������
		// ��������д���������͵� cfd ̫�죬��ʱû���ļ��� fd д��
		if (-1 == res)
		{
			perror("sendfile");
			printf("errno:%d\n", errno);
		}
	}
#endif
	close(fd);
}