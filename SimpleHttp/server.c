#include"server.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>

struct FdInfo // ��Ϊ���߳�ʱ�����̺߳����Ĵ������
{
	int fd;
	int epfd;
	pthread_t tid;
};

int initListenFd(unsigned short port)
{
	// 1.��������fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == lfd)
	{
		perror("socket\n");
		return -1;
	}
	// 2.���ö˿ڸ���
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (-1 == ret)
	{
		perror("setsockopt\n");
		return -1;
	}
	// 3.��
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*) &addr, sizeof addr);
	if (-1 == ret)
	{
		perror("bind\n");
		return -1;
	}
	// 4.���ü���
	ret = listen(lfd, 128);
	if (-1 == ret)
	{
		perror("listen\n");
		return -1;
	}
	// ����fd
	return lfd;
}

int epollRun(int lfd)
{
	// epoll ��һ������������ﴴ�����ĸ��ڵ�
	// 1.���� epoll ʵ��
	int epfd = epoll_create(1);
	if (-1 == epfd)
	{
		perror("epoll_creat\n");
		return -1;
	}
	// 2.���ļ������� lfd �� epoll ��
	struct epoll_event ev;
	ev.data.fd = lfd; // �����ļ�������
	ev.events = EPOLLIN; // �����ں˼��������
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev); // �ӵ� epoll ʵ�����ڵ��ϣ��������������
	if (-1 == ret)
	{
		perror("epoll_ctl\n");
		return -1;
	}
	// 3.�������
	struct epoll_event evs[1024]; // ���Ǵ�����������ʾ�������¼��б�
	int size = sizeof(evs) / sizeof(struct epoll_event); // �ṹ��Ԫ�ظ������� 1024
	while (1)
	{
		// �����ں˼�����϶�Ӧ���ļ���������������¼��Ƿ񱻼��������Ž� evs ������
		// ���ص� num ���ں�д�� evs ����Ч���¼�������ǰ num ��
		int num = epoll_wait(epfd, evs, size, -1); // -1 ��ʾû�м����¼�ʱһֱ������0��ʾ��������������Ϊ����ʱ��
		for (int i = 0; i < num; i++) // ����������Ч������¼�
		{
			struct FdInfo* info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
			int fd = evs[i].data.fd; // ȡ���¼����ļ�������
			info->epfd = epfd;
			info->fd = fd;
			if (fd == lfd) // �����������Ϊ������������
			{
				// ���������ӣ���ʱ accept ������������Ϊ�ں��Ѿ����������������ӵ�����
				// ֱ�ӵ��� accept ����ܻ���������Ϊ���һֱ�������������ӵ���
				//acceptClient(lfd, epfd);
				pthread_create(&info->tid, NULL, acceptClient, info); // ����ʹ�����߳�
			}
			else
			{
				// ����ͨ�ţ���Ҫ�Ǵ�������ݣ����նԶ˵�����
				//recvHttpRequest(fd, epfd);
				pthread_create(&info->tid, NULL, recvHttpRequest, info); // ����ʹ�����߳�

			}
		}
	}
	return 0;
}

//int acceptClient(int lfd, int epfd)
void* acceptClient(void* arg)
{
	struct FdInfo* info = (struct FdInfo*)arg;
	// 1.��������
	int cfd = accept(info->fd, NULL, NULL);
	if (-1 == cfd)
	{
		perror("accept\n");
		//return -1;
		return NULL;
	}
	printf("client accepted. fd: %d\n", cfd);

	// 2.�޸��ļ����������ԣ����÷�����
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);
	// 3.cfd ��� �� epoll ��
	struct epoll_event ev;
	ev.data.fd = cfd;
	ev.events = EPOLLIN | EPOLLET; // ����ָ����ֻ�����¼���ET��Ե����ģʽ��edge-triggered������Ϊд�¼����ǿ��е�
	int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev); // fd ����
	if (-1 == ret)
	{
		perror("epoll_ctl\n");
		//return -1;
		return NULL;
	}
	printf("acceptClient threadId: %ld\n", info->tid);
	free(info); // �ͷ���� malloc ������
	//return 0;
	return NULL;
}

//int recvHttpRequest(int cfd, int epfd)
void* recvHttpRequest(void* arg)
{
	struct FdInfo* info = (struct FdInfo*)arg;
	//printf("receiving data...\n");
	int len = 0, totle = 0;
	char tmp[1024] = { 0 };
	char buf[4096] = { 0 }; // GET ���󲻻�̫��
	// �������ݣ�ͬʱ���ڸ��׽����Ƿ������ģ�recv�����������ݺ󲻻��������߳�
	while ((len = recv(info->fd, tmp, sizeof tmp, 0)) > 0)
	{
		if (totle + len < sizeof buf)
		{
			memcpy(buf + totle, tmp, len);
		}
		totle += len;
	}
	// �ж������Ƿ񱻽������
	if (-1 == len && EAGAIN == errno) // û�����ݿɶ���
	{
		// ��ȡ������������
		char* pt = strstr(buf, "\r\n");
		int reqLen = pt - buf; // pt ָ�� \r������ָ�������Ϊ�������ַ�������
		buf[reqLen] = '\0'; // �ֶ���ȡ�ַ���,\r λ���ַ����ͽ���
		parseRequestLine(buf, info->fd);

	}
	else if (0 == len) // �ͻ��˶Ͽ�������,���� fd ������ɾ��
	{
		epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
		close(info->fd);
	}
	else
	{
		perror("recv\n");
	}
	printf("recvMsg threadId: %ld\n", info->tid);
	free(info); // �ͷ���� malloc ������
	//return 0;
	return NULL;
}

int parseRequestLine(const char* line, int cfd)
{

	// ����������
	// get /xxx/1.jpg http1.1
	// ע�����е� /xxx/1.jpg ��/ָ���ǿͻ���������Ǹ�Ŀ¼�������Ƿ�������Ŀ¼
	char method[12];
	char path[1024];
	sscanf(line, "%[^ ] %[^ ]", method, path); // ע���м���һ���ո�Ҫд��������ָ��������һ���ո�
	printf("method: %s , path: %s\n", method, path);
	if (0 != strcasecmp(method, "get")) // ��������ֻ���� get ����
	{
		return -1;
	}
	// ����һ���ַ�����ת���ַ�
	decodeMsg(path, path);
	printf("decoded path: %s\n", path);
	// ����ͻ�������ľ�̬��Դ��Ŀ¼���ļ���
	char* file = NULL;
	if (strcmp(path, "/") == 0)
	{
		file = "./"; // ת�������·��
	}
	else
	{
		file = path + 1; // ָ�� path ָ����һ���ַ�����ȡ file
	}
	// ��ȡ�ļ�����
	struct stat st;
	int ret = stat(file, &st);
	if (-1 == ret)
	{
		// �ļ������ڣ��ȷ���httpͷ���ٻظ�404������404.html��
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"),-1);
		sendFile("404.html", cfd);
		return 0;
	}
	// �ж��ļ�����
	if (1 == S_ISDIR(st.st_mode))
	{
		// �����Ŀ¼�����ݷ��͸��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);
	}
	else
	{
		// �ȷ���httpͷ���ٰ��ļ������ݷ��͸��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);
	}
	return 0;
}

int sendFile(const char* fileName, int cfd)
{
	// ���ļ�����һ�����ļ���һ����
	// 1.���ļ�
	int fd = open(fileName, O_RDONLY);
	// ���Դ����ļ����������Ͽ��ģ����������������ʧ�����׳��쳣
	assert(fd > 0);
	// ��������
#if 0
	while (1) // �Լ�д�����ļ��ĳ���
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);
		if (len > 0)
		{
			send(cfd, buf, len, 0);
			usleep(10); // ����10ms����ǳ���Ҫ�������ٶȹ���ᵼ���ն˳�������֮�������
		}
		else if (0 == len)
		{
			break;
		}
		else
		{
			perror("read\n");
		}
	}
#else
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
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	// ״̬��
	char buf[4096] = { 0 };
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr); // http�汾��״̬�룬״̬������linux�л�����\r\n
	// ��Ӧ�� + ����
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);
	send(cfd, buf, strlen(buf), 0);

	return 0;
}

const char* getFileType(const char* name)
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

/*
<html>
	<head>
		<title>test</title>
	</head>
	<body>
		<table>
			<tr>
				<td></td>
			</tr>
			<tr>
				<td></td>
			</tr>
		</table>
	</body>
</html>
*/

int sendDir(const char* dirName, int cfd)
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
		send(cfd, buf, strlen(buf), 0); // ֱ�ӷ������ݣ���Ϊ TCP ��ʽ����Э�鷢�Ͷ˿���һ��һ�㷢�����ն�һ��һ����
		memset(buf, 0, sizeof(buf)); // ��ջ�����
		free(namelist[i]); // �ͷŵ� scandir Ϊ namelist[i] ������ڴ�
	}
	sprintf(buf, "</table></body></html>"); // ƴ�� html β��
	send(cfd, buf, strlen(buf), 0); // ���� html β��
	free(namelist); // �ͷŵ� scandir Ϊ namelist ������ڴ�
	return 0;
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

// ����
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
