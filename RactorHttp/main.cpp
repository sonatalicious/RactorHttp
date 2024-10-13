#include "tcpServer.h"
#include "log.h"

#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char* argv[])
{
#if 0
	if (argc < 3) // 至少有3个参数
	{
		Debug("./a.out port path\n");
		return -1;
	}
	unsigned short port = atoi(argv[1]);
	chdir(argv[2]);
#else
	unsigned short port = 10000;
	chdir("/home/sonatalicious/projects/RactorHttp/resources");	// 切换服务器工作目录
#endif
	struct TcpServer* server = tcpServerInit(port, 4);
	tcpServerRun(server);
    return 0;
}