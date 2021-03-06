#include "allocator.h"

#include <stdlib.h>
#include <assert.h>

struct Allocator
{
	unsigned allocation_count;
};

Allocator *create_allocator(void *buffer, unsigned buffer_size)
{
	assert(buffer_size >= sizeof(Allocator));
	Allocator *allocator = buffer;
	allocator->allocation_count = 0;
	return allocator;
}

void destroy_allocator(Allocator *allocator)
{
	assert(allocator->allocation_count == 0);
}

void *allocator_realloc(Allocator *allocator, void *p, unsigned size, unsigned alignment)
{
	if (p && !size) { // Free
		--allocator->allocation_count;
	} else if (!p && size) { // Allocate
		++allocator->allocation_count;
	}
	
	return _aligned_realloc(p, size, alignment);
}
