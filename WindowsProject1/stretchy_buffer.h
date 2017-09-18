#pragma once

// Heavily inspired by https://github.com/nothings/stb/blob/master/stretchy_buffer.h

#define sb_create(alloc, a)	( (a) = __sbcreatef(alloc, sizeof(*(a))))
#define sb_free(a)			((a) ? (a) = allocator_realloc(sb_allocator(a), __sbraw(a), 0, 0) : 0)

#define sb_push(a,v)		(__sbmaybegrow(a,1), (a)[__sbn(a)++] = (v))
#define sb_count(a)			((a) ? __sbn(a) : 0)
#define sb_add(a,n)			(__sbmaybegrow(a,n), __sbn(a)+=(n), &(a)[__sbn(a)-(n)])
#define sb_last(a)			((a)[__sbn(a)-1])
#define sb_pop(a)			( (__sbn(a)) ? __sbn(a)-- : 0)

#define sb_allocator(a)		( __sba(a) )

#define __sbraw(a) ((unsigned __int64 *) (a) - 3)
#define __sba(a)   (struct Allocator *)__sbraw(a)[0]
#define __sbm(a)   __sbraw(a)[1]
#define __sbn(a)   __sbraw(a)[2]

#define __sbneedgrow(a,n)  ((a)==0 || __sbn(a)+(n) >= __sbm(a))
#define __sbmaybegrow(a,n) (__sbneedgrow(a,(n)) ? __sbgrow(a,n) : 0)
#define __sbgrow(a,n)      ((a) = __sbgrowf((a), (n), sizeof(*(a))))

#include <stdlib.h>

static void *__sbcreatef(struct Allocator *allocator, int item_size)
{
	const int initial_capacity = 10;
	unsigned __int64 *p = (unsigned __int64*)allocator_realloc(allocator, NULL, item_size * initial_capacity + sizeof(unsigned __int64*) * 3, 16);

	p[0] = (uintptr_t)allocator;
	p[1] = initial_capacity;
	p[2] = 0;

	return p + 3;
}

static void *__sbgrowf(void *arr, int increment, int item_size)
{
	int dbl_cur = 2 * (int)__sbm(arr);
	int min_needed = (int)sb_count(arr) + increment;
	int m = dbl_cur > min_needed ? dbl_cur : min_needed;
	struct Allocator *alloc = sb_allocator(arr);
	unsigned __int64 *p = (unsigned __int64 *)allocator_realloc(alloc, __sbraw(arr), item_size * m + sizeof(unsigned __int64*) * 3, 16);
	if (p) {
		p[1] = m;
		return p + 3;
	}
	else {
		return (void *)(3 * sizeof(int)); // try to force a NULL pointer exception later
	}
}
