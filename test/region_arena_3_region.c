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

	Region *region2 = RegionArena_GetRegion(&arena, regionTestSize);

	if (!DoesArenaSizeMatch(&arena))
		returnCode = 3;

	if (region->size != regionTestSize || region2->size != regionTestSize)
		returnCode = 4;

	Region *region3 = RegionArena_GetRegion(&arena, regionTestSize);

	if (GetActiveRegionCount(&arena) != 4)
		returnCode = 5;

	RegionArena_ReturnRegion(&arena, region2);

	if (GetActiveRegionCount(&arena) != 4)
		returnCode = 6;

	if (region->size != regionTestSize)
		returnCode = 7;

	if (!DoesArenaSizeMatch(&arena))
		returnCode = 8;

	RegionArena_Destroy(&arena);

	return returnCode;
}