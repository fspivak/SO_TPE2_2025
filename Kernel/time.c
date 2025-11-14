// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/time.h"
#include "include/interrupts.h"
#include "include/lib.h"

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
	return ticks / SECONDS_TO_TICKS;
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

void configure_timer() {
	// Configuracion del PIT (Programmable Interval Timer)
	// Puerto 0x43: Registro de comando
	// Puerto 0x40: Registro de datos del canal 0

	// Configurar modo 2 (Rate Generator) en canal 0
	// Bits: 00 (canal 0), 11 (lobyte/hibyte), 010 (modo 2), 0 (binario)
	uint8_t command = 0x36; // 00110110

	outb(0x43, command);

	// Configurar frecuencia: 1193182 Hz / 18 = ~66287 Hz
	// Esto resulta en 18 ticks por segundo = ~55.5ms por tick
	uint16_t divisor = 18;
	uint8_t low_byte = divisor & 0xFF;
	uint8_t high_byte = (divisor >> 8) & 0xFF;

	// Enviar divisor al channel 0
	outb(0x40, low_byte);
	outb(0x40, high_byte);
}