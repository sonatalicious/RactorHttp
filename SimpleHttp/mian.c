#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	//if (argc < 3) // ������3������
	//{
	//	printf("./a.out port path\n");
	//	return -1;
	//}
	printf("server started\n");
	//unsigned short port = atoi(argv[1]);
	unsigned short port = 10000;
	// �л�����������Ŀ¼
	chdir("/home/sonatalicious/projects/SimpleHttp/resources");
	//chdir(argv[2]);
	// ��ʼ�����ڼ������׽���
	int lfd = initListenFd(port); // �˿����Ϊ2**16=65535,�����5000����
	// ��������������
	epollRun(lfd);

	return 0;
}
