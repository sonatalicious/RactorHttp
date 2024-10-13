#pragma once

// ��ʼ�����ڼ������׽���
int initListenFd(unsigned short port);
// ���� epoll
int epollRun(int lfd);
// �Ϳͻ��˽�������
//int acceptClient(int lfd, int epfd);
void* acceptClient(void* arg);
// ���� http ����
//int recvHttpRequest(int cfd, int epfd);
void* recvHttpRequest(void* arg);
// ����������
int parseRequestLine(const char* line, int cfd);
// �����ļ�
int sendFile(const char* fileName, int cfd);
// ������Ӧͷ��״̬��+��Ӧͷ��
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);
// �����ļ������õ� contentType
const char* getFileType(const char* name);
// ����Ŀ¼����html��ʽ��
int sendDir(const char* dirName, int cfd);
// ���ַ�ת��Ϊ������
int hexToDec(char c);
// ����
// to �洢����֮�������, ��������, from�����������, �������
void decodeMsg(char* to, char* from);
