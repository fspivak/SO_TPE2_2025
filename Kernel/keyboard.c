#include "include/keyboard.h"
// extern de ASM para forzar scheduler inmediatamente
extern void _force_scheduler_interrupt(void);

#define BACKSPACE_P 0x0E
#define ENTER_P 0x1C
#define LSHIFT_P 0x2a
#define CAPSLOCK_P 0x3a
#define RSHIFT_P 0x36
#define LCTRL_P 0x1d
#define RCTRL_P 0xe0
#define ESC_P 0x01
#define LALT_P 0x38
#define RALT_P 0x38

#define BACKSPACE_R 0x8E
#define ENTER_R 0x9C
#define LSHIFT_R 0xaa
#define CAPSLOCK_R 0xba

#define BUFFER_LENGTH 15

char buffer[BUFFER_LENGTH];
int dim = 0;
int toPrint = 0;
int upperCase = 0;
int shift = 0;
int ctrlPressed = 0;

int ctrlCharPending = 0;

void bufferLoader(char input) {
	char release = input;
	release = release >> 7;
	char key = input & 0x7F;

	// Detectar Ctrl (make/break)
	if (key == LCTRL_P && !release) {
		ctrlPressed = 1;
		return;
	}
	if (key == LCTRL_P && release) {
		ctrlPressed = 0;
		return;
	}

	// Detectar Ctrl + C (make)
	if (key == 0x2E && !release && ctrlPressed) {
		handle_ctrl_c();
		return; // no cargar 'c' al buffer
	}

	// //quiero deterctar Ctrl + D
	// if (key ==0x20  && !release && ctrlPressed) {
	// 	handle_ctrl_d();
	// 	return; // no cargar 'd' al buffer
	// }

	// Detectar Ctrl + D (make)
	if (isCtrlPressed() && (key == 'd' || key == 'D')) {
		return; // señal de EOF
	}

	if (key == LSHIFT_P || key == RSHIFT_P) {
		shift = !release;
		return;
	}
	if (input == CAPSLOCK_P) { // veo si seria mayus
		upperCase = 1 - upperCase;
	}
	if (!(release) && !isSpecialKey(input)) { // guardo teclas apretadas no soltadas
		buffer[dim++] = input;
		dim %= BUFFER_LENGTH;
	}
}

char getKey() {
	if (toPrint == dim)
		return -1;
	unsigned char toRet = buffer[toPrint++];
	//	buffer[toPrint++] = 0;
	toPrint %= BUFFER_LENGTH;
	return toRet;
}

int isUpperCase() {
	return (upperCase) ? !shift : shift;
}

int isShiftPressed() {
	return shift;
}

int isCtrlPressed() {
	return ctrlPressed;
}

int isSpecialKey(char scancode) {
	unsigned char key = (unsigned char) scancode; /* Evitar warning de signed */
	return (key == LSHIFT_P) || (key == RSHIFT_P) || (key == LCTRL_P) || (key == RCTRL_P) || (key == LALT_P) ||
		   (key == RALT_P) || (key == CAPSLOCK_P) || (key == ESC_P) || (key == 0x57) || (key == 0x58) ||
		   (key >= 0x3B) || (key <= 1);
}