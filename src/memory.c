#include <include/memorylib/memory.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
	if (sp->used + size > sp->size)
		return NULL;
	void* ret = (void*)&sp->memory[sp->used];
	sp->used += size;
	return ret;
}
//
//void Scratchpad_Deallocate(Scratchpad* sp, u32 size)
//{
//	sp->used -= size;
//}

void Scratchpad_Reset(Scratchpad* sp)
{
	sp->used = 0;
}

void Scratchpad_Destroy(Scratchpad* sp)
{
	assert(sp != NULL);
	free(sp->memory);
	sp->memory = NULL;
	sp->size = 0;
}

static Region* GetFreeRegion(Region* const region);
static Region* GetFinalRegion(Region* const region);
static Region* SplitRegion(Region* region, u32 size);
static Region* JoinRegions(RegionArena* arena, Region* a, Region* b);
static u32 GetRegionDistanceFromHead(Region* const head, Region* current);

void RegionArena_Create(RegionArena* arena, u32 size)
{
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
}

Region* RegionArena_GetRegion(RegionArena* arena, u32 size)
{
	assert(arena->size - arena->used > sizeof(Region) + size && "Arena running out of storage!");

	Region* region = GetFreeRegion(arena->head);
	while (region->size < size)
	{
		region = GetFreeRegion(region);
	}

	region->flags |= REGION_USED;

	Region* next = SplitRegion(region, size);

	arena->used += sizeof(Region) + size;

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
	arena->used -= region->size + sizeof(Region);

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

void RegionArena_Destroy(RegionArena* arena)
{
	assert(arena->memory != NULL);
	free(arena->memory);
	arena->memory = NULL;
	arena->size = 0;
	arena->used = 0;
}

static Region* GetFreeRegion(Region* const region)
{
	assert(region != NULL);

	Region* n = region;
	while (n->next != NULL)
	{
		n = n->next;
		if (n->flags & REGION_FREE)
			break;
	}

	return n;
}

static Region* GetFinalRegion(Region* const region)
{
	assert(region != NULL);

	Region* n = region;
	while (n->next != NULL)
		n = n->next;

	return n;
}

static Region* SplitRegion(Region* region, u32 size)
{
	assert(region != NULL);

	u8* leftoverStartsAt = region->start + size;

	Region* leftover = (Region*)leftoverStartsAt;
	leftover->start = (u8*)leftover + sizeof(Region);
	leftover->next = region->next;
	leftover->flags = REGION_FREE;
	leftover->size = region->size - size - sizeof(Region);

	region->next = leftover;
	region->size = size;

	return region;
}

static Region* JoinRegions(RegionArena* arena, Region* a, Region* b)
{
	assert(a != b);
	a->size = a->size + b->size + sizeof(Region);
	a->next = b->next;

	return a;
}

static u32 GetRegionDistanceFromHead(Region* const head, Region* current)
{
	u32 ret = 0;
	while (head->next != current)
		ret++;

	return ret;
}