#include "include/scheduler.h"
#include "include/process.h"

void *scheduler(void *current_stack) {
	if (!processes_initialized()) {
		return current_stack;
	}

	return schedule(current_stack);
}

void init_scheduler() {
	init_processes();
}

extern void force_context_switch();

void force_switch() {
	force_context_switch();
}
