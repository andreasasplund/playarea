#pragma once

typedef struct Allocator Allocator;

Allocator *create_allocator(void *buffer, unsigned buffer_size);
void destroy_allocator(Allocator *allocator);

void *allocator_realloc(Allocator *allocator, void *p, unsigned size, unsigned alignment);
