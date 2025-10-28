#include "scheduler/include/process.h"
#include <keyboard.h>
#include <lib.h>
#include <stdint.h>
#include <time.h>
#include <videoDriver.h>

static void int_20();
static void int_21();

// Declaracion externa del scheduler
extern void *scheduler(void *current_stack);

void irqDispatcher(uint64_t irq) {
	if (irq == 0) {
		int_20();
	}
	if (irq == 1) {
		int_21();
	}
}

void int_20() {
	timer_handler();
	// El scheduler sera llamado desde assembly despues del timer_handler
}

void int_21() {
	char character = returnKBOutputInterrupt();
	bufferLoader(character);
}

void handle_ctrl_c(void) {
	process_id_t pid = get_current_pid();
	if (pid > 0) {
		kill_process(pid);
	}
}