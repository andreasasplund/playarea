#pragma once

typedef struct FibersSystem FibersSystem;
typedef struct Allocator Allocator;

FibersSystem *fibers_system_create(Allocator *allocator, unsigned n_fibers);
void fibers_system_destroy(Allocator *allocator, FibersSystem *fibers_system);

typedef void (*FibersSystemJobEntry)(void *data);
typedef struct FibersSystemJobDecl
{
	FibersSystemJobEntry job_entry;
	void *callback_data;
} FibersSystemJobDecl;

typedef struct FibersSystemCounter FibersSystemCounter;

void fibers_system_run_jobs(FibersSystem *fibers_system, FibersSystemJobDecl *job_declarations, unsigned n_job_declarations, FibersSystemCounter **job_counter);
void fibers_system_wait_for_counter(FibersSystem *fibers_system, FibersSystemCounter *job_counter, unsigned value);