#ifndef MEMORY_REGION_ARENA_TEST_HELPERS_H
#define MEMORY_REGION_ARENA_TEST_HELPERS_H

#include <include/memorylib/memory.h>

#include <stdlib.h>

#include <assert.h>

#include <stdbool.h>

#define SCRATCHPAD_SIZE_BYTES 8 * 1024 * 1024

#define REGION_ARENA_TOTAL_SIZE_BYTES 16 * 1024 * 1024

u32 GetActiveRegionCount(const RegionArena* const arena)
{
	assert(arena->head != NULL);

	Region* iter = arena->head;
	u32 count = 1;

	while (iter->next != NULL)
	{
		count++;
		assert(iter != iter->next);
		iter = iter->next;
	}

	return count;
}

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