#include "fibers_system.h"

#include <stdlib.h>
#include <Windows.h>
#include <assert.h>

#include "allocator.h"
#include "stretchy_buffer.h"

typedef struct FibersSystemJob
{
	FibersSystemCounter *counter;
	FibersSystemJobDecl declaration;
} FibersSystemJob;

typedef struct FiberStruct
{
	FibersSystem *fibers_system;
	LPVOID fiber;
	unsigned entry;

	FibersSystemJob current_job;
} FiberStruct;

typedef struct FibersSystemCounter
{
	LONG counter;
	unsigned entry;
} FibersSystemCounter;

typedef struct FibersSystemWaitList
{
	LPVOID fiber;
	FibersSystemCounter *counter;
	unsigned expected_value;
} FibersSystemWaitList;

typedef struct FibersSystem
{
	FiberStruct *fibers;
	unsigned *free_fibers;
	FibersSystemWaitList *wait_list;

	FibersSystemJob *jobs;

	FibersSystemCounter *job_counters;
	unsigned *free_job_counters;

	LPVOID main_fiber;
	LONG add_variable;
} FibersSystem;

FiberStruct *allocate_fiber(FibersSystem *fibers_system)
{
	assert(sb_count(fibers_system->free_fibers) > 0);
	const unsigned last_slot = sb_count(fibers_system->free_fibers) - 1;
	unsigned free_fiber = sb_last(fibers_system->free_fibers);
	fibers_system->free_fibers[last_slot] = 0xffffffffu;
	sb_pop(fibers_system->free_fibers);
	return &fibers_system->fibers[free_fiber];
}

void free_fiber(FibersSystem *fibers_system, FiberStruct *fiber)
{
	fiber->current_job.counter = NULL;
	fiber->current_job.declaration.job_entry = NULL;
	fiber->current_job.declaration.job_data = NULL;

	assert(sb_count(fibers_system->free_fibers) < sb_count(fibers_system->fibers));
	unsigned free_fiber = fiber->entry;
	sb_push(fibers_system->free_fibers, free_fiber);
}

void fibers_system_do_work(FibersSystem *fibers_system)
{
	// Go over all fibers in wait list and see if any is done, if so, switch to that fiber.
	const unsigned n_wait_list_entries = sb_count(fibers_system->wait_list);
	for (unsigned int i = 0; i < n_wait_list_entries; ++i) {
		FibersSystemWaitList *waiting = &fibers_system->wait_list[i];
		unsigned counter_value = InterlockedAdd(&waiting->counter->counter, 0);
		if (waiting->expected_value != counter_value)
			continue;

		FibersSystemWaitList done = *waiting;
		fibers_system->wait_list[i] = sb_last(fibers_system->wait_list);
		sb_pop(fibers_system->wait_list);

		SwitchToFiber(done.fiber);
		return;
	}

	// Couldn't find a fiber that is done, proceed with consuming jobs from the job queue.
	while (sb_count(fibers_system->jobs)) {
		FibersSystemJob job = sb_last(fibers_system->jobs);
		sb_pop(fibers_system->jobs);
		FiberStruct *fiber = allocate_fiber(fibers_system);
		fiber->current_job = job;

		if (fiber->fiber == GetCurrentFiber())
			return;
		SwitchToFiber(fiber->fiber);
	}
}

void fiber_entry_point(LPVOID fiber_param)
{
	FiberStruct *fiber_struct = fiber_param;

	while (1) {
		FibersSystemJob current_job = fiber_struct->current_job;

		if (current_job.counter && current_job.declaration.job_entry && current_job.declaration.job_data) {
			(*current_job.declaration.job_entry)(current_job.declaration.job_data);
			InterlockedDecrement(&current_job.counter->counter);
		}

		free_fiber(fiber_struct->fibers_system, fiber_struct);
		fibers_system_do_work(fiber_struct->fibers_system);
	}
}

FibersSystem *fibers_system_create(Allocator *allocator, unsigned n_fibers)
{
	FibersSystem* fibers_system = allocator_realloc(allocator, NULL, sizeof(FibersSystem), 16);
	sb_create(allocator, fibers_system->fibers, n_fibers);
	sb_add(fibers_system->fibers, n_fibers);
	sb_create(allocator, fibers_system->wait_list, n_fibers);

	sb_create(allocator, fibers_system->free_fibers, n_fibers);
	for (unsigned i = 0; i < n_fibers; ++i)
		sb_push(fibers_system->free_fibers, i);

	sb_create(allocator, fibers_system->jobs, 10);

	sb_create(allocator, fibers_system->job_counters, 256);
	sb_add(fibers_system->job_counters, sb_capacity(fibers_system->job_counters));
	for (unsigned i = 0; i < sb_count(fibers_system->job_counters); ++i)
		fibers_system->job_counters[i].entry = i;

	sb_create(allocator, fibers_system->free_job_counters, 256);
	for (unsigned i = 0; i < 256; ++i)
		sb_push(fibers_system->free_job_counters, i);

	fibers_system->main_fiber = ConvertThreadToFiber(NULL);
	for (unsigned i = 0; i < n_fibers; ++i) {
		fibers_system->fibers[i].fiber = CreateFiber(0, fiber_entry_point, &fibers_system->fibers[i]);
		fibers_system->fibers[i].fibers_system = fibers_system;
		fibers_system->fibers[i].entry = i;
	}

	fibers_system->add_variable = 0;

	InterlockedIncrement(&fibers_system->add_variable);

	return fibers_system;
}

void fibers_system_destroy(Allocator *allocator, FibersSystem *fibers_system)
{
	const unsigned n_fibers = sb_count(fibers_system->fibers);
	for (unsigned i = 0; i < n_fibers; ++i) {
		DeleteFiber(fibers_system->fibers[i].fiber);
	}

	BOOL result = ConvertFiberToThread();
	assert(result == TRUE);
	(void)result;

	sb_free(fibers_system->fibers);
	sb_free(fibers_system->wait_list);
	sb_free(fibers_system->free_fibers);
	sb_free(fibers_system->jobs);
	sb_free(fibers_system->job_counters);
	sb_free(fibers_system->free_job_counters);
	allocator_realloc(allocator, fibers_system, 0, 0);
}

FibersSystemCounter *allocate_job_counter(FibersSystem *fibers_system)
{
	assert(sb_count(fibers_system->free_job_counters) > 0);
	
	unsigned free_counter = sb_last(fibers_system->free_job_counters);
	sb_pop(fibers_system->free_job_counters);
	return &fibers_system->job_counters[free_counter];
}

void free_job_counter(FibersSystem *fibers_system, FibersSystemCounter *counter)
{
	assert(sb_count(fibers_system->free_job_counters) < sb_count(fibers_system->job_counters));
	unsigned free_counter = counter->entry;
	sb_push(fibers_system->free_job_counters, free_counter);
}

void fibers_system_run_jobs(FibersSystem *fibers_system, FibersSystemJobDecl *job_declarations, unsigned n_job_declarations, FibersSystemCounter **job_counter)
{
	assert(sb_count(fibers_system->free_fibers) >= n_job_declarations);

	*job_counter = allocate_job_counter(fibers_system);
	(*job_counter)->counter = n_job_declarations;

	for (unsigned i = 0; i < n_job_declarations; ++i) {
		FibersSystemJob job = { *job_counter, job_declarations[i] };
		sb_push(fibers_system->jobs, job);
	}
}

void fibers_system_wait_for_counter(FibersSystem *fibers_system, FibersSystemCounter *job_counter, unsigned value)
{
	unsigned counter_value = InterlockedAdd(&job_counter->counter, 0);
	if (value == counter_value)
		return;

	FibersSystemWaitList wait_list_entry = { .fiber = GetCurrentFiber(), .counter = job_counter, .expected_value = value };
	sb_push(fibers_system->wait_list, wait_list_entry);

	fibers_system_do_work(fibers_system);
	free_job_counter(fibers_system, job_counter);
}