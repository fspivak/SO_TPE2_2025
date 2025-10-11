#include "include/interrupts.h"

static unsigned long ticks = 0;
int last_seconds = 0;

extern uint8_t manually_triggered_timer_interrupt;

void timer_handler() {
	if (manually_triggered_timer_interrupt) {
		manually_triggered_timer_interrupt = 0;
		return;
	}
	else {
		ticks++;
	}
}

int ticks_elapsed() {
	return ticks;
}

int seconds_elapsed() {
	return ticks / 18;
}

// funcion sleep en ticks
// termina siendo la syscall sleep
void sleep(int time) {
	int current_ticks = ticks;
	while (current_ticks + time >= ticks) {
		_hlt();
	}
}

uint64_t getMiSe() {
	return ticks;
}