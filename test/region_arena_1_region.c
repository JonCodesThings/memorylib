#include <include/memorylib/memory.h>

#include "region_arena_test_helpers.h"

#include <stdlib.h>

const u32 regionTestSize = 1024;

int main()
{
	int returnCode = 0;
	RegionArena arena;

	RegionArena_Create(&arena, REGION_ARENA_TOTAL_SIZE_BYTES);

	Region *region = RegionArena_GetRegion(&arena, regionTestSize);

	if (region->size != regionTestSize)
		returnCode = 1;

	if (!DoesArenaSizeMatch(&arena))
		returnCode = 2;

	RegionArena_ReturnRegion(&arena, region);

	if (!DoesArenaSizeMatch(&arena))
		returnCode = 3;

	RegionArena_Destroy(&arena);

	return returnCode;
}