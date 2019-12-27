/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers
Copyright (C) 2019-.... FAETHER / Etherr
*/

#ifndef __ZZONE_H
#define __ZZONE_H

/*
 memory allocation


H_??? The hunk manages the entire memory block given.  It must be
contiguous.  Memory can be allocated from either the low or high end in a
stack fashion.  The only way memory is released is by resetting one of the
pointers.

Hunk allocations should be given a name, so the Hunk_Print () function
can display usage.

Hunk allocations are guaranteed to be 16 byte aligned.

The video buffers are allocated high to avoid leaving a hole underneath
server allocations when changing to a higher video mode.


Z_??? Zone memory functions used for small, dynamic allocations like text
strings from command input.  There is only about 48K for it, allocated at
the very bottom of the hunk.

Cache_??? Cache memory is for objects that can be dynamically loaded and
can usefully stay persistant between levels.  The size of the cache
fluctuates from level to level.

To allocate a cachable object


Temp_??? Temp memory is used for file loading and surface caching.  The size
of the cache memory is adjusted so that there is a minimum of 512k remaining
for temp memory.


------ Top of Memory -------

high hunk allocations

<--- high hunk reset point held by vid

video buffer

z buffer

surface cache

<--- high hunk used

cachable memory

<--- low hunk used

client and server low hunk allocations

<-- low hunk reset point held by host

startup hunk allocations

Zone block

----- Bottom of Memory -----



*/

#include "startup.h"

extern unsigned char* stack_mem;
typedef struct cache_user_s
{
	void	*data;
} cache_user_t;

void* VEtherAlloc(void* pusd, size_t size, size_t align, VkSystemAllocationScope allocationScope);
void* VEtherRealloc(void* pusd, void* porg, size_t size, size_t align, VkSystemAllocationScope allocationScope);
void VEtherFree(void* pusd, void* ptr);

namespace zone
{
float Q_atof (const char *str);
int Q_strlen(const char *str);
void Q_memcpy(void *dest, const void *src, size_t count);
size_t q_strlcpy (char *dst, const char *src, size_t siz);
char *Z_Strdup (const char *s);
int Q_strncmp(const char *s1, const char *s2, int count);
int Q_strcmp(const char *s1, const char *s2);
void Q_memset(void *dest, int fill, size_t count);
size_t Q_sstrlen (const char* s) ;
void Q_strcpy(char *dest, const char *src);
void Q_strcat(char *dest, const char *src);

__attribute__((noinline)) void stack_alloc(int size);
void stack_clear(int size);

void Memory_Init (void *buf, int size);

void Z_Free (void *ptr);
void *Z_TagMalloc (int size, int tag, int align);
void *Z_Malloc (int size); // returns 0 filled memory.
void *Z_Realloc (void *ptr, int size, int align);
char *Z_Strdup (const char *s);
void Z_TmpExec();
void MemPrint();

void *Hunk_Alloc (int size);		// returns 0 filled memory
void *Hunk_AllocName (int size, const char *name);
void *Hunk_HighAllocName (int size, const char *name);
void *Hunk_ShrinkHigh(int size);
char *Hunk_Strdup (const char *s, const char *name);

int	Hunk_LowMark (void);
void Hunk_FreeToLowMark (int mark);

int	Hunk_HighMark (void);
void Hunk_FreeToHighMark (int mark);

void *Hunk_TempAlloc (int size);

void Hunk_Check (void);

void Cache_Flush (void);

void *Cache_Check (cache_user_t *c);
// returns the cached data, and moves to the head of the LRU list
// if present, otherwise returns NULL

void Cache_Free (cache_user_t *c, bool freetextures); //johnfitz -- added second argument

void *Cache_Alloc (cache_user_t *c, int size, const char *name);
// Returns NULL if all purgable data was tossed and there still
// wasn't enough room.

void Cache_Report (void);

}

#endif	/* __ZZONE_H */

