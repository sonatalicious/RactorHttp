#pragma once

/*
相当于一个管理 channel 的映射
*/

struct ChannelMap
{
	//struct Channel* list[];
	struct Channel** list; // 直接以空间换时间，用一个大列表当做 map
	int size; // 记录指针指向的数组的元素总个数
};

// 初始化
struct ChannelMap* channelMapInit(int size);
// 清空 map
void channelMapClear(struct ChannelMap* map);
// 重新分配内存空间（扩容）
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize);