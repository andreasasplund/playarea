#pragma once

struct Allocator;

struct Allocator *create_allocator(void *buffer, unsigned buffer_size);
void destroy_allocator(struct Allocator *allocator);

void *allocator_allocate(struct Allocator *allocator, unsigned size, unsigned alignment);
void allocator_free(struct Allocator *allocator, void *buffer);