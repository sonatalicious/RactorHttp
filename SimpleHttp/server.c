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

struct FdInfo // 作为多线程时创造线程函数的传入参数
{
	int fd;
	int epfd;
	pthread_t tid;
};

int initListenFd(unsigned short port)
{
	// 1.创建监听fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == lfd)
	{
		perror("socket\n");
		return -1;
	}
	// 2.设置端口复用
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (-1 == ret)
	{
		perror("setsockopt\n");
		return -1;
	}
	// 3.绑定
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
	// 4.设置监听
	ret = listen(lfd, 128);
	if (-1 == ret)
	{
		perror("listen\n");
		return -1;
	}
	// 返回fd
	return lfd;
}

int epollRun(int lfd)
{
	// epoll 是一个红黑树，这里创建它的根节点
	// 1.创建 epoll 实例
	int epfd = epoll_create(1);
	if (-1 == epfd)
	{
		perror("epoll_creat\n");
		return -1;
	}
	// 2.将文件描述符 lfd 上 epoll 树
	struct epoll_event ev;
	ev.data.fd = lfd; // 设置文件描述符
	ev.events = EPOLLIN; // 拜托内核检查新连接
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev); // 加到 epoll 实例根节点上，并设置相关属性
	if (-1 == ret)
	{
		perror("epoll_ctl\n");
		return -1;
	}
	// 3.持续检测
	struct epoll_event evs[1024]; // 这是传出参数，表示监听的事件列表
	int size = sizeof(evs) / sizeof(struct epoll_event); // 结构体元素个数，即 1024
	while (1)
	{
		// 拜托内核检测树上对应的文件描述符，的相关事件是否被激活，激活则放进 evs 数组里
		// 返回的 num 是内核写到 evs 中有效的事件个数，前 num 个
		int num = epoll_wait(epfd, evs, size, -1); // -1 表示没有激活事件时一直阻塞，0表示不阻塞，其他则为阻塞时间
		for (int i = 0; i < num; i++) // 遍历所有有效激活的事件
		{
			struct FdInfo* info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
			int fd = evs[i].data.fd; // 取出事件的文件描述符
			info->epfd = epfd;
			info->fd = fd;
			if (fd == lfd) // 如果此描述符为监听的描述符
			{
				// 创建新连接，此时 accept 不会阻塞，因为内核已经告诉我们有新连接到达了
				// 直接调用 accept 则可能会阻塞，因为其会一直阻塞到有新连接到达
				//acceptClient(lfd, epfd);
				pthread_create(&info->tid, NULL, acceptClient, info); // 这里使用子线程
			}
			else
			{
				// 数据通信，主要是处理读数据，接收对端的数据
				//recvHttpRequest(fd, epfd);
				pthread_create(&info->tid, NULL, recvHttpRequest, info); // 这里使用子线程

			}
		}
	}
	return 0;
}

//int acceptClient(int lfd, int epfd)
void* acceptClient(void* arg)
{
	struct FdInfo* info = (struct FdInfo*)arg;
	// 1.建立连接
	int cfd = accept(info->fd, NULL, NULL);
	if (-1 == cfd)
	{
		perror("accept\n");
		//return -1;
		return NULL;
	}
	printf("client accepted. fd: %d\n", cfd);

	// 2.修改文件描述符属性，设置非阻塞
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);
	// 3.cfd 添加 到 epoll 中
	struct epoll_event ev;
	ev.data.fd = cfd;
	ev.events = EPOLLIN | EPOLLET; // 这里指定了只检测读事件，ET边缘触发模式（edge-triggered），因为写事件总是可行的
	int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev); // fd 上树
	if (-1 == ret)
	{
		perror("epoll_ctl\n");
		//return -1;
		return NULL;
	}
	printf("acceptClient threadId: %ld\n", info->tid);
	free(info); // 释放这块 malloc 的数据
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
	char buf[4096] = { 0 }; // GET 请求不会太大
	// 接收数据，同时由于该套接字是非阻塞的，recv函数读完数据后不会阻塞该线程
	while ((len = recv(info->fd, tmp, sizeof tmp, 0)) > 0)
	{
		if (totle + len < sizeof buf)
		{
			memcpy(buf + totle, tmp, len);
		}
		totle += len;
	}
	// 判断数据是否被接收完成
	if (-1 == len && EAGAIN == errno) // 没有数据可读了
	{
		// 提取并解析请求行
		char* pt = strstr(buf, "\r\n");
		int reqLen = pt - buf; // pt 指向 \r，两个指针相减即为请求行字符串长度
		buf[reqLen] = '\0'; // 手动截取字符串,\r 位置字符串就结束
		parseRequestLine(buf, info->fd);

	}
	else if (0 == len) // 客户端断开了连接,将此 fd 从树上删除
	{
		epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
		close(info->fd);
	}
	else
	{
		perror("recv\n");
	}
	printf("recvMsg threadId: %ld\n", info->tid);
	free(info); // 释放这块 malloc 的数据
	//return 0;
	return NULL;
}

int parseRequestLine(const char* line, int cfd)
{

	// 解析请求行
	// get /xxx/1.jpg http1.1
	// 注意其中的 /xxx/1.jpg 的/指的是客户端请求的那个目录，而不是服务器根目录
	char method[12];
	char path[1024];
	sscanf(line, "%[^ ] %[^ ]", method, path); // 注意中间有一个空格，要写出来，让指针跳过这一个空格
	printf("method: %s , path: %s\n", method, path);
	if (0 != strcasecmp(method, "get")) // 我们这里只处理 get 请求
	{
		return -1;
	}
	// 处理一下字符串的转义字符
	decodeMsg(path, path);
	printf("decoded path: %s\n", path);
	// 处理客户端请求的静态资源（目录或文件）
	char* file = NULL;
	if (strcmp(path, "/") == 0)
	{
		file = "./"; // 转换成相对路径
	}
	else
	{
		file = path + 1; // 指向 path 指针下一个字符，提取 file
	}
	// 获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (-1 == ret)
	{
		// 文件不存在，先发送http头，再回复404（发送404.html）
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"),-1);
		sendFile("404.html", cfd);
		return 0;
	}
	// 判断文件类型
	if (1 == S_ISDIR(st.st_mode))
	{
		// 把这个目录的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);
	}
	else
	{
		// 先发送http头，再把文件的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);
	}
	return 0;
}

int sendFile(const char* fileName, int cfd)
{
	// 打开文件，读一部分文件发一部分
	// 1.打开文件
	int fd = open(fileName, O_RDONLY);
	// 断言打开了文件，断言是严苛的，不允许程序发生错误，失败则抛出异常
	assert(fd > 0);
	// 发送数据
#if 0
	while (1) // 自己写发送文件的程序
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);
		if (len > 0)
		{
			send(cfd, buf, len, 0);
			usleep(10); // 休眠10ms，这非常重要，发送速度过快会导致收端出现意料之外的问题
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
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	// 状态行
	char buf[4096] = { 0 };
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr); // http版本，状态码，状态描述，linux中换行是\r\n
	// 响应行 + 空行
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);
	send(cfd, buf, strlen(buf), 0);

	return 0;
}

const char* getFileType(const char* name)
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
		send(cfd, buf, strlen(buf), 0); // 直接发送数据，因为 TCP 流式传输协议发送端可以一点一点发，接收端一点一点收
		memset(buf, 0, sizeof(buf)); // 清空缓冲区
		free(namelist[i]); // 释放掉 scandir 为 namelist[i] 分配的内存
	}
	sprintf(buf, "</table></body></html>"); // 拼接 html 尾端
	send(cfd, buf, strlen(buf), 0); // 发送 html 尾端
	free(namelist); // 释放掉 scandir 为 namelist 分配的内存
	return 0;
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

// 解码
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
