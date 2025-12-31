#include <include/memorylib/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef MEMORYLIB_DEBUG
#define memorylib_debug(str) printf("%s\n", str);
#define memorylib_debug_only(...) __VA_ARGS__
#define memorylib_debugf(format, ...) printf(format, __VA_ARGS__); printf("\n");
#define memorylib_assert(cond, str) do { if (!(cond)) { memorylib_debug(str); } assert(cond); } while(0);
#define memorylib_assertf(cond, format, ...) do { if (!(cond)) { memorylib_debugf(format, __VA_ARGS__); } assert(cond); } while(0);
#else
#define memorylib_debug(str)
#define memorylib_debug_only(...)
#define memorylib_debugf(format, ...)
#define memorylib_assert(cond, format)
#define memorylib_assertf(cond, format, ...)
#endif

const u8 REGION_FREE = 0;
const u8 REGION_USED = 1 << 0;

void Scratchpad_Create(Scratchpad* scratchpad, u32 size)
{
	scratchpad->memory = NULL;
	scratchpad->memory = (u8*)malloc(size);
	scratchpad->size = size;
	scratchpad->used = 0;
}

void* Scratchpad_Allocate(Scratchpad* sp, u32 size)
{
	assert(sp->used + size < sp->size);
	memorylib_assertf(sp->used + size < sp->size, "Failed to allocate: out of space in scratchpad! %d/%d", sp->used, sp->size);
	if (sp->used + size > sp->size)
		return NULL;
	void* ret = (void*)&sp->memory[sp->used];
	sp->used += size;
	return ret;
}

void Scratchpad_Reset(Scratchpad* sp)
{
	sp->used = 0;
}

void Scratchpad_Destroy(Scratchpad* sp)
{
	memorylib_assert(sp != NULL, "Scratchpad was NULL!");
	free(sp->memory);
	sp->memory = NULL;
	sp->size = 0;
}

static Region* GetFreeRegion(Region* const region);
memorylib_debug_only(static Region* GetFinalRegion(Region* const region);)
static Region* SplitRegion(Region* region, u32 size);
static Region* JoinRegions(RegionArena* arena, Region* a, Region* b);
memorylib_debug_only(static u32 GetRegionDistanceFromHead(Region* const head, Region* current);)
Region* FindRegion(RegionArena* arena, u8* start);

void RegionArena_Create(RegionArena* arena, u32 size)
{
	memorylib_debugf("Allocating region arena of size %d", size);
	arena->memory = NULL;
	arena->memory = (u8*)malloc(size);
	arena->size = 0;

	if (arena->memory == NULL)
		return;

	arena->size = size;
	arena->used = 0;

	arena->head = (Region*)arena->memory;
	arena->head->size = arena->size - sizeof(Region);
	arena->head->start = arena->memory + sizeof(Region);
	arena->head->next = NULL;
	memorylib_debugf("Allocator head region initialized with size %d and neighbor %p", arena->head->size, arena->head->next);
}

u8* RegionArena_Allocate(RegionArena* arena, u32 size)
{
	Region *region = RegionArena_GetRegion(arena, size);
	memorylib_assert(region != NULL, "Region is NULL!");
	memorylib_debugf("Allocated new region starting at %p of size %d bytes", region->start, region->size);
	return region->start;
}

Region* RegionArena_GetRegion(RegionArena* arena, u32 size)
{
	memorylib_assert(arena->size - arena->used > sizeof(Region) + size, "Arena running out of storage!");

	Region* region = GetFreeRegion(arena->head);
	while (region->size < size)
	{
		region = GetFreeRegion(region);
	}

	region->flags |= REGION_USED;

	Region* next = SplitRegion(region, size);
	if (next == NULL)
	{
		return NULL;
	}

	arena->used += sizeof(Region) + region->size;

	return region;
}

Region* RegionArena_ResizeRegion(RegionArena* arena, Region* region, u32 size)
{
	if (region->size == size)
		return region;

	if (region->next != NULL)
	{
		Region *next = region->next;
		if (next->flags & REGION_FREE)
		{
			u32 sizeDiff = size - region->size;

			region->size += sizeDiff;

			memcpy(next + size, next, sizeof(Region));
			region->next = next + size;
			return region;
		}
	}

	Region* realloc = RegionArena_GetRegion(arena, size);
	memcpy(realloc->start, region->start, region->size);
	RegionArena_ReturnRegion(arena, region);

	return realloc;
}


void RegionArena_ReturnRegion(RegionArena* arena, Region* region)
{
	memorylib_assert(region != NULL, "Region is NULL!");
	arena->used -= region->size + sizeof(Region);
	memorylib_debugf("Returning region at %p with size %d!", region->start, region->size);

	Region* neighborRegion = region->next;

	region->flags = REGION_FREE;

	if (region->next != NULL && !(region->next->flags & REGION_USED))
		JoinRegions(arena, region, region->next);

	neighborRegion = arena->head;

	while (neighborRegion->next != region && neighborRegion->next != NULL)
	{
		neighborRegion = neighborRegion->next;
	}

	if (neighborRegion != arena->head && neighborRegion != region && (neighborRegion->flags & REGION_FREE))
		JoinRegions(arena, neighborRegion, region);
}

void RegionArena_Deallocate(RegionArena* arena, u8* alloc)
{
	Region* region = FindRegion(arena, alloc);
	RegionArena_ReturnRegion(arena, region);
}

void RegionArena_Destroy(RegionArena* arena)
{
	memorylib_assert(arena->memory != NULL, "Region arena's memory was NULL!");
	free(arena->memory);
	arena->memory = NULL;
	arena->size = 0;
	arena->used = 0;
}

Region* FindRegion(RegionArena* arena, u8* start)
{
	memorylib_assert(arena != NULL, "Arena is NULL!");
	Region* n = arena->head;

	while (n->next != NULL)
	{
		if (n->start == start)
		{
			memorylib_debugf("Found region (start:%p size:%d next:%p)", n->start, n->size, n->next);
			return n;
		}
		n = n->next;
	}

	memorylib_debugf("Failed to find region starting at %p!", n->start);
	return NULL;
}

static Region* GetFreeRegion(Region* const region)
{
	memorylib_assert(region != NULL, "Region is NULL!");

	Region* n = region;
	while (n->next != NULL)
	{
		n = n->next;
		if (n->flags & REGION_USED)
			continue;
		break;
	}

	memorylib_debugf("Found free region (start:%p size:%d next:%p)", n->start, n->size, n->next);
	return n;
}

#ifdef MEMORYLIB_DEBUG
static Region* GetFinalRegion(Region* const region)
{
	memorylib_assert(region != NULL, "Region is NULL!");

	Region* n = region;
	while (n->next != NULL)
		n = n->next;

	return n;
}
#endif

static Region* SplitRegion(Region* region, u32 size)
{
	memorylib_assert(region != NULL, "Region is NULL!");
	memorylib_debugf("Splitting region (start:%p size:%d next:%p)", region->start, region->size, region->next);

	u32 leftoverSize = region->size - size;
	memorylib_debugf("Remainder left over from allocation %d", leftoverSize);

	if (leftoverSize > sizeof(Region))
	{
		u8* leftoverStartsAt = region->start + size;
		Region* leftover = (Region*)leftoverStartsAt;
		leftover->start = (u8*)leftover + sizeof(Region);
		leftover->next = region->next;
		leftover->flags = REGION_FREE;
		leftover->size = region->size - size - sizeof(Region);

		region->next = leftover;
		region->size = size;
		memorylib_debugf("Region (start:%p size:%d next:%p) split to form new neighbor region (start:%p size:%d next:%p)", region->start, region->size, region->next, leftover->start, leftover->size, leftover->next);
	}
	else
	{
		memorylib_debugf("Region (start:%p size:%d next:%p) left unsplit: not enough room for region header in remainder memory", region->start, region->size, region->next);
	}

	return region;
}

static Region* JoinRegions(RegionArena* arena, Region* a, Region* b)
{
	memorylib_assert(a != b, "Region pointers were the same!");
	memorylib_debugf("Merging regions (start:%p size:%d next:%p) and (start:%p size:%d next:%p) to form new region of size %d (removing redundant header of size:%d) with neighbor %p", a->start, a->size, a->next, b->start, b->size, b->next, a->size + b->size + (u32)sizeof(Region), (u32)sizeof(Region), b->next);
	a->size = a->size + b->size + sizeof(Region);
	a->next = b->next;

	return a;
}

#ifdef MEMORYLIB_DEBUG
static u32 GetRegionDistanceFromHead(Region* const head, Region* current)
{
	u32 ret = 0;
	while (head->next != current)
		ret++;

	return ret;
}
#endif
