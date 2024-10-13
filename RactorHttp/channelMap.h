#pragma once

/*
�൱��һ������ channel ��ӳ��
*/

struct ChannelMap
{
	//struct Channel* list[];
	struct Channel** list; // ֱ���Կռ任ʱ�䣬��һ�����б��� map
	int size; // ��¼ָ��ָ��������Ԫ���ܸ���
};

// ��ʼ��
struct ChannelMap* channelMapInit(int size);
// ��� map
void channelMapClear(struct ChannelMap* map);
// ���·����ڴ�ռ䣨���ݣ�
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize);