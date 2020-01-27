/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers
Copyright (C) 2019-.... FAETHER / Etherr
*/

#include "zone.h"
#include <cstddef>
#include <csetjmp>
#include "flog.h"
#include "window.h"
#include <mutex>
/*
==============================================================================

					MEMORY/STRING MANIPULATION

==============================================================================
*/

unsigned char* stack_mem = nullptr;

static std::mutex zmtx[3];
//This is a hook to c++ style allocations.
//VEther will filter it's c++ libraries like glsl compiler
//thorough this memory zone instead.
void* operator new(size_t size)
{
	zmtx[0].lock();
	void* p = zone::Z_TagMalloc(size, 1);
	ASSERT(p,"operator new failed.");
	zmtx[0].unlock();
	return p;
}

void operator delete(void* p)
{
	zmtx[0].lock();
	zone::Z_Free(p, 1);
	zmtx[0].unlock();
	return;
}

void operator delete(void* p, size_t size)
{
	zmtx[0].lock();
	zone::Z_Free(p, 1);
	zmtx[0].unlock();
	// Here this will keep the memory alive but does not is free.
	// I question size parameter.
	// "If present, the std::size_t size argument must equal the
	// size argument passed to the allocation function that returned ptr."
	// Concluded to ignore size alltogether.
	return;
}

void* VEtherAlloc(void* pusd, size_t size, size_t align, VkSystemAllocationScope allocationScope)
{
	zmtx[1].lock();
	void* p = zone::Z_TagMalloc(size, 2);
	//void* p = malloc(size);
	ASSERT(p,"VEtherAlloc failed.");
	zmtx[1].unlock();
	return p;
}

void* VEtherRealloc(void* pusd, void* porg, size_t size, size_t align, VkSystemAllocationScope allocationScope)
{
	zmtx[1].lock();
	void* p = zone::Z_Realloc(porg, size, 2);
	//void* p = realloc(porg, size);
	ASSERT(p,"VEtherRealloc failed.");
	zmtx[1].unlock();
	return p;
}

void VEtherFree(void* pusd, void* ptr)
{
	//free(ptr);
	zmtx[1].lock();
	zone::Z_Free(ptr, 2);
	zmtx[1].unlock();
	return;
}

void* Bt_alloc(size_t size)
{
	zmtx[2].lock();
	void* p = zone::Z_TagMalloc(size, 0);
	ASSERT(p,"Bt_alloc failed.");
	zmtx[2].unlock();
	return p;
}

void Bt_free(void* ptr)
{
	zmtx[2].lock();
	zone::Z_Free(ptr, 0);
	zmtx[2].unlock();
	return;
}


namespace zone
{

float Q_atof (const char *str)
{
	double		val;
	int		sign;
	int		c;
	int	decimal, total;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
			break;
		val = val*10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val*sign;
}


size_t q_strlcpy (char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0)
	{
		while (--n != 0)
		{
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0)
	{
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

void Q_memset(void *dest, int fill, size_t count)
{
	size_t		i;

	if ( (((size_t)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill<<8) | (fill<<16) | (fill<<24);
		for (i = 0; i < count; i++)
			((int *)dest)[i] = fill;
	}
	else
		for (i = 0; i < count; i++)
			((unsigned char *)dest)[i] = fill;
}

void Q_memcpy(void *dest, const void *src, size_t count)
{
	size_t		i;

	if (( ( (size_t)dest | (size_t)src | count) & 3) == 0 )
	{
		count >>= 2;
		for (i = 0; i < count; i++)
			((int *)dest)[i] = ((int *)src)[i];
	}
	else
		for (i = 0; i < count; i++)
			((unsigned char *)dest)[i] = ((unsigned char *)src)[i];
}

int Q_memcmp(const void *m1, const void *m2, size_t count)
{
	while(count)
	{
		count--;
		if (((unsigned char *)m1)[count] != ((unsigned char *)m2)[count])
			return -1;
	}
	return 0;
}

void Q_strcpy(char *dest, const char *src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

void Q_strncpy (char *dest, const char *src, int count)
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

int Q_strlen(const char *str)
{
	int		count;

	count = 0;
	while (str[count])
		count++;

	return count;
}

size_t Q_sstrlen (const char* s)
{
	register const char* i;
	for(i=s; *i != ' '; ++i);
	return (i-s);
}

char *Q_strrchr(const char *s, char c)
{
	int len = Q_strlen(s);
	s += len;
	while (len--)
	{
		if (*--s == c)
			return (char *)s;
	}
	return NULL;
}

void Q_strcat(char *dest, const char *src)
{
	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
}

int Q_strcmp(const char *s1, const char *s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_strncmp(const char *s1, const char *s2, int count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

/*
==============================================================================

						STACK MEMORY ALLOCATION
It is important to understand that this memory allocation is expremely dangerous
and can possibly cause Undefined Behavior (UB).

It can be used on a global scope.
Last size is stored in stack_mem pointer value;
Make sure to clean up the stack before or after, if needed.

==============================================================================
*/

__attribute__((noinline)) void stack_alloc(int size)
{
ret:
	;
	static int val = 0;
	if(val == 1)
	{
		//we have to give a reason for compiler
		//to know that this is a valid return function
		//and is not UB.
		val = 0;
		return;
	}
	if(stack_mem != nullptr)
	{
		unsigned char mem[*((int*)stack_mem)+size];
		*((int*)mem - sizeof(int)) = size;
		*((int*)mem - (sizeof(int)*2)) = reinterpret_cast<uintptr_t>(stack_mem);
		stack_mem = mem;
	}
	else
	{
		unsigned char mem[size];
		*((int*)mem - sizeof(int)) = size;
		stack_mem = mem;
	}
	val = 1;
	goto ret;
}

void stack_clear(int size)
{
	if(stack_mem != nullptr)
	{
		Q_memset(stack_mem, 0, size);
	}
}

/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks. Unless Z_UpdateRover has yet to process.

The rover must be pointing at an empty block. Otherwise will be NULL

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

#define	DYNAMIC_SIZE	(1024 * 1024)

#define	ZONEID	0x1d4a11
#define MINFRAGMENT	64

typedef struct memblock_s
{
	int	size;		// including the header and possibly tiny fragments
	int	tag;		// a tag of 0 is a free block
#ifdef DEBUG
	int	id;		// should be ZONEID
#endif
	struct	memblock_s	*next, *prev;
} memblock_t;

typedef struct
{
	int		size;		// total bytes malloced, including header
	memblock_t	blocklist;	// start / end cap for linked list
	memblock_t	*rover;
	memblock_t      *staging_rover;
} memzone_t;

void Cache_FreeLow (int new_low_hunk);
void Cache_FreeHigh (int new_high_hunk);
static void Memory_InitZone (memzone_t *zone, int size);

#define NUM_ZONES 3
static memzone_t	*mainzone[NUM_ZONES];
int zsizes[NUM_ZONES];

/*
========================
Z_UpdateRover

Keeps the rover pointing at the largest free block.
Updated every frame in separate thread.
========================
*/
void Z_UpdateRover()
{
	memblock_t dummy;
	dummy.size = 0;
	memblock_t* max = &dummy;
	memblock_t	*block;
	memzone_t *zone;
	memblock_t* other;
	for(;;)
	{
		for(uint8_t i = 0; i<NUM_ZONES; i++)
		{
			zone = mainzone[i];
			for (block = zone->blocklist.next ; ; block = block->next)
			{
				if (block->next == &zone->blocklist)
				{
					break;			// all blocks have been hit
				}
				
				if (!block->tag && !block->next->tag)
				{
					other = block->next;
					block->size += other->size;
					block->next = other->next;
					block->next->prev = block;
				}
				if (!block->tag && !block->prev->tag)
				{
					// merge with previous free block
					other = block->prev;
					other->size += block->size;
					other->next = block->next;
					other->next->prev = other;
				}
			}
			for (block = zone->blocklist.next ; ; block = block->next)
			{
				if (block->next == &zone->blocklist)
				{
					break;			// all blocks have been hit
				}
				
				if(!block->tag)
				{
					if(block->size > max->size)
					{
						max = block;
					}
				}
			}
				
			if(max != &dummy)
			{
				if(zone->rover->size < max->size)
				{
				//	debug("Old rover for zone %d, %p, %d, %d",i, zone->rover, zone->rover->size, zone->rover->tag);
					zone->rover->tag = 0;
					max->tag = 2;
					zone->rover = max;
				//	debug("New rover for zone %d, %p, %d, %d",i, zone->rover, zone->rover->size, zone->rover->tag);
				}
			}
			max = &dummy;
			if(deltatime < 0.02f)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}
}

/*
========================
Z_Free
========================
*/

void Z_Free (void *ptr, uint8_t zoneid)
{
	memblock_t	*block;
	if (!ptr)
	{
#ifdef DEBUG
		static bool once = true; //silence
		if(once)
		{
			info("Z_Free: NULL pointer");
			once = false;
		}
#endif
		return;
	}
	block = (memblock_t *) ( (unsigned char *)ptr - sizeof(memblock_t));
#ifdef DEBUG
	if (block->id != ZONEID)
	{
		warn("Z_Free: freed a pointer without ZONEID");
		return;
	}
	if (block->tag == 0)
	{
		warn("Z_Free: freed a freed pointer zoneid: %d %p %d", zoneid, ptr, block->size);
	}
#endif
	block->tag = 0;		// mark as free
}

void Z_TmpExec()
{
	static memzone_t* _zone = nullptr;
	if(!_zone)
	{
		_zone = mainzone[0];
		mainzone[0] = (memzone_t *) Hunk_TempAlloc(DYNAMIC_SIZE);
		Memory_InitZone(mainzone[0], DYNAMIC_SIZE);
	}
	else
	{
		Hunk_HighMark();
		mainzone[0] = _zone;
		_zone = nullptr;
	}
}

/*
========================
Z_Print
========================
*/
void Z_Print (uint8_t zoneid)
{
	memblock_t	*block;
	memzone_t *zone = mainzone[zoneid];
	uint64_t sum = 0;
	uint64_t actual_size = 0;
	uint64_t idx = 0;
	debug("**********************************");
	debug("zone size: %i  location: %p, id: %d", zsizes[zoneid], zone, zoneid);

	for (block = zone->blocklist.next ; ; block = block->next)
	{
		if (block->next == &zone->blocklist)
		{
			break;			// all blocks have been hit
		}
		if(block->tag)
		{
			sum += block->size;
		}
		actual_size += block->size;
		//debug("block:%p    size:%7i    tag:%d asize:%d idx:%d", block, block->size, block->tag, actual_size, idx);
		idx++;
		if ( (unsigned char *)block + block->size != (unsigned char *)block->next)
		{
			fatal("ERROR: block size does not touch the next block");
			fatal("%p != %p", block+block->size, block->next);
		}
// There are no longer an error but it's okay to have them.
// Keep for reference.
/* 		if ( block->next->prev != block)
		{
			fatal("ERROR: next block doesn't have proper back link");
		}
		if (!block->tag && !block->next->tag)
		{
			fatal("ERROR: two consecutive free blocks");
		} */
	}
	debug("Rover: %p size: %d tag: %d", mainzone[zoneid]->rover,  mainzone[zoneid]->rover->size, mainzone[zoneid]->rover->tag);
	debug("Memory used: %dB out of %dB | %0.2fMB out of %0.2fMB",
	      sum,  zsizes[zoneid], (float)sum/(float)(1024*1024),
	      (float)zsizes[zoneid]/(float)(1024*1024));
	debug("Block extent actual: %dB | %0.2fMB", actual_size, (float)actual_size/float(1024*1024));

}

void *Z_TagMalloc (int size, uint8_t zoneid)
{
	memblock_t	*newblock, *base;

	size += sizeof(memblock_t);	// account for size of block header
#ifdef DEBUG
	size += sizeof(int);		// space for memory trash tester
#endif
	size = (size + (sizeof(max_align_t)-1)) & -sizeof(max_align_t);
	//alignment must be consistent through out the allocator.

	base = mainzone[zoneid]->rover;
	//p("%d %d %d %p %d",base->size, base->tag, zoneid, base, size);
	if(base->tag == 1 || base->size < size)
	{
		return NULL;
	}
	
	// there will be a free fragment after the allocated block
	newblock = (memblock_t *) ((unsigned char *)base + size );
	newblock->size = base->size - size;
	newblock->tag = 2;			// free block
	newblock->prev = base;
	newblock->next = base->next;
	newblock->next->prev = newblock;
	mainzone[zoneid]->rover = newblock;	// next allocation will start looking here

	base->next = newblock;
	base->size = size;
	base->tag = 1;				// no longer a free block

#ifdef DEBUG
	// marker for memory trash testing
	base->id = ZONEID;
	newblock->id = ZONEID;
	*(int *)((unsigned char *)base + base->size - sizeof(int)) = ZONEID;
#endif

	return (void *) ((unsigned char *)base + sizeof(memblock_t));
}

/*
========================
Z_Malloc
========================
*/
void *Z_Malloc (int size, uint8_t zoneid)
{
	void	*buf;

	//Z_CheckHeap ();

	buf = Z_TagMalloc (size, zoneid);
	if (!buf)
	{
		fatal("Z_Malloc: failed on allocation of %i bytes", size);
	}
	Q_memset (buf, 0, size);

	return buf;
}

/*
========================
Z_Realloc
========================
*/
void *Z_Realloc(void *ptr, int size, uint8_t zoneid)
{
	int old_size;
	void *old_ptr;
	memblock_t *block;

	if (!ptr)
	{
		return Z_Malloc (size, zoneid);
	}
	block = (memblock_t *) ((unsigned char *) ptr - sizeof (memblock_t));

#ifdef DEBUG
	if (block->id != ZONEID)
	{
		fatal ("Z_Realloc: realloced a pointer without ZONEID");
	}
	if (block->tag == 0)
	{
		fatal ("Z_Realloc: realloced a freed pointer");
	}
#endif
	old_size = block->size;

#ifdef DEBUG
	old_size -= sizeof(int); /* see Z_TagMalloc() */
#endif
	old_size -= ((int)sizeof(memblock_t));
	old_ptr = ptr;

	Z_Free (ptr, zoneid);
	ptr = Z_TagMalloc (size, zoneid);
	if (!ptr)
	{
		fatal("Z_Realloc: failed on allocation of %i bytes", size);
	}
	if (ptr != old_ptr)
		memmove (ptr, old_ptr, q_min(old_size, size));
	if (old_size < size)
		Q_memset ((unsigned char *)ptr + old_size, 0, size - old_size);

	return ptr;
}

char *Z_Strdup (const char *s)
{
	size_t sz = strlen(s) + 1;
	char *ptr = (char *) Z_Malloc (sz);
	memcpy (ptr, s, sz);
	return ptr;
}

//============================================================================

#define	HUNK_SENTINAL	0x1df001ed

#define HUNKNAME_LEN	24
typedef struct
{
	int		sentinal;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[HUNKNAME_LEN];
} hunk_t;

unsigned char	*hunk_base;
int		hunk_size;

int		hunk_low_used;
int		hunk_high_used;

bool	hunk_tempactive;
int		hunk_tempmark;

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void Hunk_Check (void)
{
	hunk_t	*h;

	for (h = (hunk_t *)hunk_base ; (unsigned char *)h != hunk_base + hunk_low_used ; )
	{
		if (h->sentinal != HUNK_SENTINAL)
		{
			debug ("Hunk_Check: trahsed sentinal");
		}
		if (h->size < (int) sizeof(hunk_t) || h->size + (unsigned char *)h - hunk_base > hunk_size)
		{
			debug ("Hunk_Check: bad size");
		}
		h = (hunk_t *)((unsigned char *)h+h->size);
	}
}


/*
===================
Hunk_AllocName
===================
*/
void *Hunk_AllocName (int size, const char *name)
{
	hunk_t	*h;

	if (size < 0)
	{
		fatal("Hunk_Alloc: bad size: %i", size);
	}

	size = sizeof(hunk_t) + ((size+15)&~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		fatal("Hunk_Alloc: failed on %i bytes",size);
	}
	h = (hunk_t *)(hunk_base + hunk_low_used);
	hunk_low_used += size;

	Cache_FreeLow(hunk_low_used);

	Q_memset (h, 0, size);

	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	q_strlcpy (h->name, name, HUNKNAME_LEN);

	return (void *)(h+1);
}

/*
===================
Hunk_Alloc
===================
*/
void *Hunk_Alloc (int size)
{
	return Hunk_AllocName (size, "unknown");
}

int	Hunk_LowMark (void)
{
	return hunk_low_used;
}

void Hunk_FreeToLowMark (int mark)
{
	if (mark < 0 || mark > hunk_low_used)
	{
		fatal("Hunk_FreeToLowMark: bad mark %i", mark);
	}
	Q_memset (hunk_base + mark, 0, hunk_low_used - mark);
	hunk_low_used = mark;
}

void Hunk_FreeToHighMark (int mark)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}
	if (mark < 0 || mark > hunk_high_used)
	{
		fatal ("Hunk_FreeToHighMark: bad mark %i", mark);
	}
	Q_memset (hunk_base + hunk_size - hunk_high_used, 0, hunk_high_used - mark);
	hunk_high_used = mark;
}

int	Hunk_HighMark (void)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}

	return hunk_high_used;
}


void* Hunk_HighPos()
{

	hunk_t	*h;
	h = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	return (void*) (h+1);
}

/*
===================
Hunk_HighAllocName
===================

Scheme is as follows:

| 0x0 | <- hunklow (used 0, goes down)        |
| 0x4 |                                       |  ^
| 0x8 |                                       v  |
| 0xC | <- hunkhigh  (used 0, goes up)           |

Direction tells how memory grows during allocation only.
Now in the cpu, real direction always goes down because of direction flag.

Sample:

| 0x0 |
| 0x4 | <- size gets writen (both hunks work the same way)
| 0x8 | <- sentinal right after
| 0xC | <- 16 byte alignment
| 0x10 | <- pointer retured by allocator
| 0x14 | <- data you use goes down

Sample only for demo, not an exact model;

*/

void *Hunk_HighAllocName (int size, const char *name)
{
	hunk_t	*h;

	if (size < 0)
		fatal("Hunk_HighAllocName: bad size: %i", size);

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

	size = sizeof(hunk_t) + ((size+15)&~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		fatal("Hunk_HighAlloc: failed on %i bytes\n",size);
		return NULL;
	}

	hunk_high_used += size;
	Cache_FreeHigh (hunk_high_used);

	h = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	Q_memset (h, 0, size);
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	q_strlcpy (h->name, name, HUNKNAME_LEN);

	return (void *)(h+1);
}

/*@size how much to shrink?*/
void *Hunk_ShrinkHigh(int size)
{
	size = ((size+15)&~15) - sizeof(hunk_t);
	hunk_t	*h;
	hunk_t *nh;
	h = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	hunk_high_used -= size;
	nh = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	nh->size = h->size - size;
	nh->sentinal = HUNK_SENTINAL;
	q_strlcpy (nh->name, h->name, HUNKNAME_LEN);
	Q_memcpy(nh+1, h+1, nh->size); //have to relocate the block
	h->size = 0;
	h->sentinal = 0;
	Q_memset(h->name, 0, HUNKNAME_LEN);
	return (void *)(nh+1);
}

/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void *Hunk_TempAlloc (int size)
{
	void	*buf;

	size = (size+15)&~15;

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

	hunk_tempmark = Hunk_HighMark ();

	buf = Hunk_HighAllocName (size, "temp");

	hunk_tempactive = true;

	return buf;
}

char *Hunk_Strdup (const char *s, const char *name)
{
	size_t sz = strlen(s) + 1;
	char *ptr = (char *) Hunk_AllocName (sz, name);
	memcpy (ptr, s, sz);
	return ptr;
}

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/

#define CACHENAME_LEN	32
typedef struct cache_system_s
{
	int			size;		// including this header
	cache_user_t		*user;
	char			name[CACHENAME_LEN];
	struct cache_system_s	*prev, *next;
	struct cache_system_s	*lru_prev, *lru_next;	// for LRU flushing
} cache_system_t;

cache_system_t *Cache_TryAlloc (int size, bool nobottom);

cache_system_t	cache_head;

/*
===========
Cache_Move
===========
*/
void Cache_Move ( cache_system_t *c)
{
	cache_system_t		*new_cs;

// we are clearing up space at the bottom, so only allocate it late
	new_cs = Cache_TryAlloc (c->size, true);
	if (new_cs)
	{
		fatal("cache_move ok\n");

		Q_memcpy ( new_cs+1, c+1, c->size - sizeof(cache_system_t) );
		new_cs->user = c->user;
		Q_memcpy (new_cs->name, c->name, sizeof(new_cs->name));
		Cache_Free (c->user, false); //johnfitz -- added second argument
		new_cs->user->data = (void *)(new_cs+1);
	}
	else
	{
		fatal ("cache_move failed\n");

		Cache_Free (c->user, true); // tough luck... //johnfitz -- added second argument
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeLow (int new_low_hunk)
{
	cache_system_t	*c;

	while (1)
	{
		c = cache_head.next;
		if (c == &cache_head)
			return;		// nothing in cache at all
		if ((unsigned char *)c >= hunk_base + new_low_hunk)
			return;		// there is space to grow the hunk
		Cache_Move ( c );	// reclaim the space
	}
}

/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeHigh (int new_high_hunk)
{
	cache_system_t	*c, *prev;

	prev = NULL;
	while (1)
	{
		c = cache_head.prev;
		if (c == &cache_head)
			return;		// nothing in cache at all
		if ( (unsigned char *)c + c->size <= hunk_base + hunk_size - new_high_hunk)
			return;		// there is space to grow the hunk
		if (c == prev)
			Cache_Free (c->user, true);	// didn't move out of the way //johnfitz -- added second argument
		else
		{
			Cache_Move (c);	// try to move it
			prev = c;
		}
	}
}

void Cache_UnlinkLRU (cache_system_t *cs)
{
	if (!cs->lru_next || !cs->lru_prev)
		fatal ("Cache_UnlinkLRU: NULL link");

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

void Cache_MakeLRU (cache_system_t *cs)
{
	if (cs->lru_next || cs->lru_prev)
		fatal ("Cache_MakeLRU: active link");

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
cache_system_t *Cache_TryAlloc (int size, bool nobottom)
{
	cache_system_t	*cs, *new_cs;

// is the cache completely empty?

	if (!nobottom && cache_head.prev == &cache_head)
	{
		if (hunk_size - hunk_high_used - hunk_low_used < size)
			fatal ("Cache_TryAlloc: %i is greater then free hunk", size);

		new_cs = (cache_system_t *) (hunk_base + hunk_low_used);
		Q_memset (new_cs, 0, sizeof(*new_cs));
		new_cs->size = size;

		cache_head.prev = cache_head.next = new_cs;
		new_cs->prev = new_cs->next = &cache_head;

		Cache_MakeLRU (new_cs);
		return new_cs;
	}

// search from the bottom up for space

	new_cs = (cache_system_t *) (hunk_base + hunk_low_used);
	cs = cache_head.next;

	do
	{
		if (!nobottom || cs != cache_head.next)
		{
			if ( (unsigned char *)cs - (unsigned char *)new_cs >= size)
			{
				// found space
				Q_memset (new_cs, 0, sizeof(*new_cs));
				new_cs->size = size;

				new_cs->next = cs;
				new_cs->prev = cs->prev;
				cs->prev->next = new_cs;
				cs->prev = new_cs;

				Cache_MakeLRU (new_cs);

				return new_cs;
			}
		}

		// continue looking
		new_cs = (cache_system_t *)((unsigned char *)cs + cs->size);
		cs = cs->next;

	}
	while (cs != &cache_head);

// try to allocate one at the very end
	if ( hunk_base + hunk_size - hunk_high_used - (unsigned char *)new_cs >= size)
	{
		Q_memset (new_cs, 0, sizeof(*new_cs));
		new_cs->size = size;

		new_cs->next = &cache_head;
		new_cs->prev = cache_head.prev;
		cache_head.prev->next = new_cs;
		cache_head.prev = new_cs;

		Cache_MakeLRU (new_cs);

		return new_cs;
	}

	return NULL;		// couldn't allocate
}

/*
============
Cache_Flush

Throw everything out, so new data will be demand cached
============
*/
void Cache_Flush (void)
{
	while (cache_head.next != &cache_head)
		Cache_Free ( cache_head.next->user, true); // reclaim the space //johnfitz -- added second argument
}

/*
============
Cache_Print

============
*/
void Cache_Print (void)
{
	cache_system_t	*cd;

	for (cd = cache_head.next ; cd != &cache_head ; cd = cd->next)
	{
		debug("%8i : %s", cd->size, cd->name);
	}
}

/*
============
Cache_Report

============
*/
void Cache_Report (void)
{
	Cache_Print();
	debug("%0.2fMB data cache", (hunk_size - hunk_high_used - hunk_low_used) / (float)(1024*1024));
	debug("%0.2fMB data cache avail", hunk_size/(float)(1024*1024));
	debug("%0.2fMB hunk_high_used | %0.2fMB hunk_low_used", hunk_high_used/(float)(1024*1024), (hunk_low_used-DYNAMIC_SIZE)/(float)(1024*1024));
}

/*
============
Cache_Init

============
*/
void Cache_Init (void)
{
	cache_head.next = cache_head.prev = &cache_head;
	cache_head.lru_next = cache_head.lru_prev = &cache_head;

	//Cmd_AddCommand ("flush", Cache_Flush);
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
void Cache_Free (cache_user_t *c, bool freetextures) //johnfitz -- added second argument
{
	cache_system_t	*cs;

	if (!c->data)
	{
		error("Cache_Free: not allocated");
	}

	cs = ((cache_system_t *)c->data) - 1;

	cs->prev->next = cs->next;
	cs->next->prev = cs->prev;
	cs->next = cs->prev = NULL;

	c->data = NULL;

	Cache_UnlinkLRU (cs);
}



/*
==============
Cache_Check
==============
*/
void *Cache_Check (cache_user_t *c)
{
	cache_system_t	*cs;

	if (!c->data)
		return NULL;

	cs = ((cache_system_t *)c->data) - 1;

// move to head of LRU
	Cache_UnlinkLRU (cs);
	Cache_MakeLRU (cs);

	return c->data;
}


/*
==============
Cache_Alloc
==============

template for future:

cache_user_t cn = {};
void* a = zone::Cache_Alloc(&cn, 200, "test");
*/
void *Cache_Alloc (cache_user_t *c, int size, const char *name)
{
	cache_system_t	*cs;

	if (c->data)
	{
		fatal("Cache_Alloc: allready allocated");
	}
	if (size <= 0)
	{
		fatal("Cache_Alloc: size %i", size);
	}

	size = (size + sizeof(cache_system_t) + 15) & ~15;

// find memory for it
	while (1)
	{
		cs = Cache_TryAlloc (size, false);
		if (cs)
		{
			q_strlcpy (cs->name, name, CACHENAME_LEN);
			c->data = (void *)(cs+1);
			cs->user = c;
			break;
		}

		// free the least recently used candedat
		if (cache_head.lru_prev == &cache_head)
		{
			fatal("Cache_Alloc: out of memory"); // not enough memory at all
		}

		Cache_Free (cache_head.lru_prev->user, true); //johnfitz -- added second argument
	}

	return Cache_Check (c);
}

//============================================================================


static void Memory_InitZone (memzone_t *zone, int size)
{
	memblock_t	*block;

// set the entire zone to one free block

	zone->blocklist.next = zone->blocklist.prev = block =
	                           (memblock_t *)( (unsigned char *)zone + sizeof(memzone_t) );
	zone->blocklist.tag = 1;	// in use block
#ifdef DEBUG
	zone->blocklist.id = 0;
#endif
	zone->blocklist.size = 0;
	zone->rover = block;

	block->prev = block->next = &zone->blocklist;
	block->tag = 2;			// free block
#ifdef DEBUG
	block->id = ZONEID;
#endif
	block->size = size - sizeof(memzone_t);
}

void MemPrint()
{
	Z_Print(0);
	Z_Print(1);
	Z_Print(2);
	Cache_Report();
	Hunk_Check();
}

/*
========================
Memory_Init
========================
*/
void Memory_Init (void *buf, int size)
{
	int zonesize = DYNAMIC_SIZE;

	hunk_base = (unsigned char *) buf;
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;

	Cache_Init();
	zsizes[0] = 32*zonesize;
	mainzone[0] = (memzone_t *) Hunk_AllocName (zsizes[0], "mainzone");
	Memory_InitZone (mainzone[0], zsizes[0]);

	zsizes[1] = 128*zonesize;
	mainzone[1] = (memzone_t *) Hunk_AllocName (zsizes[1], "newzone");
	Memory_InitZone (mainzone[1], zsizes[1]);

	zsizes[2] = 32*zonesize;
	mainzone[2] = (memzone_t *) Hunk_AllocName (zsizes[2], "vulkanzone");
	Memory_InitZone (mainzone[2], zsizes[2]);
}

} //namespace zone
