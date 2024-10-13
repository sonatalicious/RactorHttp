#include "workerThread.h"

#include <stdio.h>
#include <pthread.h>

int workerThreadInit(struct WorkerThread* thread, int index)
{
	thread->evLoop = NULL;
	thread->threadID = 0;
	sprintf(thread->name, "SubThread - %d", index);
	pthread_mutex_init(&thread->mutex, NULL);
	pthread_cond_init(&thread->cond, NULL);
	return 0;
}
// ���̵߳Ļص�����
void* subThreadRunning(void* arg)
{
	struct WorkerThread* thread = (struct WorkerThread*)arg;
	pthread_mutex_lock(&thread->mutex); // �Թ�����Դ thread->evLoop ����
	thread->evLoop = eventLoopInitEx(thread->name);
	pthread_mutex_unlock(&thread->mutex); // ����
	pthread_cond_signal(&thread->cond); // evLoop �Ѿ��ɹ���ʼ�������ڽ�����̵߳�����
	eventLoopRun(thread->evLoop);
	return NULL;
}

void workerThreadRun(struct WorkerThread* thread) // �������̵߳��õĺ���
{
	// �������߳�
	pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
	// �������̣߳��õ�ǰ��������ֱ�ӽ���
	pthread_mutex_lock(&thread->mutex); // thread->evLoop �ǹ�����Դ�����Լ���
	while (NULL == thread->evLoop) // Ҫ��֤���߳�ȡ�� thread->evLoop ��Դʱ��������ֻ�û��ʼ����Ϊ NULL �����
	{
		pthread_cond_wait(&thread->cond, &thread->mutex); // evLoop ��ʼ�����֮ǰһֱ����
	}
	pthread_mutex_unlock(&thread->mutex);
	// ��Щ���������������Ĳ���ʹ�� workerThreadRun �����ڽ���֮ǰ��evLoop �ض����ɹ���ʼ��
	// �ڶ��߳��У��õ�������Դ����Ҫ�������̼߳��ĳЩ������ĳ���Ⱥ�˳������Ҫ�߳�����������Ҫ�����������ź�
}
