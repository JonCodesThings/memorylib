#ifndef MEMORY_REGION_ARENA_TEST_HELPERS_H
#define MEMORY_REGION_ARENA_TEST_HELPERS_H

//extern "C"
//{
	//#include <include/oslib/platform.h>
//}

#include <include/memorylib/memory.h>

#include <stdlib.h>

#include <assert.h>

#include <stdbool.h>

u32 GetTotalSizeOfAllRegions(RegionArena *arena)
{
	Region *region = arena->head;
	u32 totalSize = 0;

	while (region != NULL)
	{
		totalSize += region->size;
		assert(region != region->next);
		region = region->next;
	}

	return totalSize;
}

bool DoesArenaSizeMatch(RegionArena *arena)
{
	u32 count = GetActiveRegionCount(arena);

	u32 size = GetTotalSizeOfAllRegions(arena);

	return size == arena->size - (sizeof(Region) * count);
}

#endif