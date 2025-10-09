#include "include/scheduler.h"
#include "include/process.h"

// Funcion llamada desde el timer interrupt (assembly)
void *scheduler(void *current_stack) {
	if (!processes_initialized()) {
		return current_stack;
	}

	return schedule(current_stack);
}

// Inicializa el scheduler
void init_scheduler() {
	init_processes();
}

// Fuerza un context switch (usado por yield)
extern void force_context_switch();

void force_switch() {
	force_context_switch();
}
