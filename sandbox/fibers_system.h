#pragma once

typedef struct FibersSystem FibersSystem;
typedef struct Allocator Allocator;

FibersSystem *create_fibers_system(Allocator *allocator);
void destroy_fibers_system(Allocator *allocator, FibersSystem *fibers_system);