#ifndef MEMORYLIB_MEMORY_H
#define MEMORYLIB_MEMORY_H

#if OSLIB
#include <include/oslib/platform.h>
#else
#ifdef _MSC_VER
#if _WIN32
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;
#endif
#endif
#endif

#define SCRATCHPAD_SIZE_BYTES 8 * 1024 * 1024

#define REGION_ARENA_TOTAL_SIZE_BYTES 16 * 1024 * 1024

typedef struct Scratchpad
{
	u8* memory;
	u32 size;
	u32 used;
} Scratchpad;

void Scratchpad_Create(Scratchpad* sp, u32 size);

void* Scratchpad_Allocate(Scratchpad* sp, u32 size);

void Scratchpad_Destroy(Scratchpad* sp);

void Scratchpad_Reset(Scratchpad* sp);

extern const u8 REGION_FREE;
extern const u8 REGION_USED;

typedef struct Region
{
	struct Region *next;
	u8 *start;
	u32 size;
	u8 flags;
} Region;

typedef struct RegionArena
{
	u8 *memory;
	u32 size;
	u32 used;
	Region *head;
} RegionArena;

void RegionArena_Create(RegionArena* arena, u32 size);

Region* RegionArena_GetRegion(RegionArena* arena, u32 size);

Region *RegionArena_ResizeRegion(RegionArena* arena, Region* region, u32 size);

void RegionArena_ReturnRegion(RegionArena* arena, Region* region);

void RegionArena_Destroy(RegionArena* arena);

#endif