#include "allocator.h"

#include <stdlib.h>
#include <assert.h>

struct Allocator
{
	unsigned allocation_count;
};

struct Allocator *create_allocator(void *buffer, unsigned buffer_size)
{
	assert(buffer_size >= sizeof(struct Allocator));
	struct Allocator *allocator = (struct Allocator*)buffer;
	allocator->allocation_count = 0;
	return allocator;
}

void destroy_allocator(struct Allocator *allocator)
{
	assert(allocator->allocation_count == 0);
}

void *allocator_allocate(struct Allocator *allocator, unsigned size, unsigned alignment)
{
	++allocator->allocation_count;
	return _aligned_malloc(size, alignment);
}

void allocator_free(struct Allocator *allocator, void *buffer)
{
	--allocator->allocation_count;
	_aligned_free(buffer);
}