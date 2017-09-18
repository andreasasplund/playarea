#pragma once

struct Allocator;

struct Allocator *create_allocator(void *buffer, unsigned buffer_size);
void destroy_allocator(struct Allocator *allocator);

void *allocator_realloc(struct Allocator *allocator, void *p, unsigned size, unsigned alignment);
