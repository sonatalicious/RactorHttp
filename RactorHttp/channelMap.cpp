#include "channelMap.h"
#include "channel.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct ChannelMap* channelMapInit(int size)
{
	struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
	map->size = size;
	map->list = (struct Channel**)malloc(size * sizeof(struct Channel*));
	memset(map->list, 0, size * sizeof(struct Channel*));
	return map;
}

void channelMapClear(struct ChannelMap* map)
{
	if (NULL != map)
	{
		for (int i = 0; i < map->size; i++)
		{
			if (NULL != map->list[i])
			{
				free(map->list[i]);
			}
		}
		free(map->list);
		map->list = NULL;
	}
	map->size = 0;
}

bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize) // 第三个参数是 channel 指针的大小
{
	if (map->size < newSize)
	{
		int curSize = map->size;
		// 容量每次扩大一倍
		while (curSize < newSize)
		{
			curSize *= 2;
		}
		// 扩容 realloc
		struct Channel** temp = (struct Channel**)realloc(map->list, curSize * unitSize);
		if (NULL == temp)
		{
			return false;
		}
		map->list = temp;
		// 这个 memset 较难理解，其实就是从最后一个元素的后一个元素位置开始，将分配好的剩下的内存初始化为0
		memset(map->list[map->size], 0, (curSize - map->size) * unitSize);
		map->size = curSize;
	}

	return true;
}
