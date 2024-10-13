#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	//if (argc < 3) // 至少有3个参数
	//{
	//	printf("./a.out port path\n");
	//	return -1;
	//}
	printf("server started\n");
	//unsigned short port = atoi(argv[1]);
	unsigned short port = 10000;
	// 切换服务器工作目录
	chdir("/home/sonatalicious/projects/SimpleHttp/resources");
	//chdir(argv[2]);
	// 初始化用于监听的套接字
	int lfd = initListenFd(port); // 端口最大为2**16=65535,最好是5000以上
	// 启动服务器程序
	epollRun(lfd);

	return 0;
}
