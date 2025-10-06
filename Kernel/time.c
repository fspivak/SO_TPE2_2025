#include "include/interrupts.h"

static unsigned long ticks = 0;
int last_seconds = 0;

void timer_handler() {
	ticks++;
}

int ticks_elapsed() {
	return ticks;
}

int seconds_elapsed() {
	return ticks / 18;
}

//funcion sleep en ticks
//termina siendo la syscall sleep
void sleep(int time){
	int current_ticks=ticks;
	while(current_ticks+time>=ticks){
		_hlt();
	}
}

uint64_t getMiSe(){
	return ticks;
}