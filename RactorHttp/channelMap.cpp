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

bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize) // ������������ channel ָ��Ĵ�С
{
	if (map->size < newSize)
	{
		int curSize = map->size;
		// ����ÿ������һ��
		while (curSize < newSize)
		{
			curSize *= 2;
		}
		// ���� realloc
		struct Channel** temp = (struct Channel**)realloc(map->list, curSize * unitSize);
		if (NULL == temp)
		{
			return false;
		}
		map->list = temp;
		// ��� memset ������⣬��ʵ���Ǵ����һ��Ԫ�صĺ�һ��Ԫ��λ�ÿ�ʼ��������õ�ʣ�µ��ڴ��ʼ��Ϊ0
		memset(map->list[map->size], 0, (curSize - map->size) * unitSize);
		map->size = curSize;
	}

	return true;
}
