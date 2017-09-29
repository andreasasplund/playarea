#include "fibers_system.h"
#include "allocator.h"
#include <stdlib.h>

typedef struct FibersSystem
{
	unsigned blapp;
} FibersSystem;

FibersSystem *create_fibers_system(Allocator *allocator)
{
	return allocator_realloc(allocator, NULL, sizeof(FibersSystem), 16);
}

void destroy_fibers_system(Allocator *allocator, FibersSystem *fibers_system)
{
	allocator_realloc(allocator, fibers_system, 0, 0);
}