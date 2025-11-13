#include "include/stdinout.h"
#include "include/interrupts.h"
#include "include/keyboard.h"
#include "include/videoDriver.h"
#include <stddef.h>

char conversionArray[] = {
	[0x01] = -1, // escape no tiene caracter imprimible
	[0x02] = '1',  [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',	[0x07] = '6',
	[0x08] = '7',  [0x09] = '8', [0x0A] = '9', [0x0B] = '0', [0x0C] = '\'', [0x0D] = '=',
	[0x0E] = 8,	   // backspace no es imprimible
	[0x0F] = '\t', // tab no es imprimible
	[0x10] = 'q',  [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',	[0x15] = 'y',
	[0x16] = 'u',  [0x17] = 'i', [0x18] = 'o', [0x19] = 'p', [0x1A] = '[',	[0x1B] = ']',
	[0x1C] = '\n', // enter no es imprimible
	[0x1D] = -1,   // left control no es imprimible
	[0x1E] = 'a',  [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',	[0x23] = 'h',
	[0x24] = 'j',  [0x25] = 'k', [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
	[0x2A] = -1, // left shift no es imprimible
	[0x2B] = '\\', [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v',	[0x30] = 'b',
	[0x31] = 'n',  [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '-',
	[0x36] = -1,  // right shift no es imprimible
	[0x37] = '*', // (keypad) * es imprimible
	[0x38] = -1,  // left alt no es imprimible
	[0x39] = ' ', // Espacio
	[0x3A] = -1,  // CapsLock no es imprimible
};

/* Tabla de conversion con Shift presionado */
char shiftConversionArray[] = {
	[0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%', [0x07] = '&', [0x08] = '/',
	[0x09] = '(', [0x0A] = ')', [0x0B] = '=', [0x0C] = '?', [0x0D] = '"', [0x1A] = '^', [0x1B] = '*',
	[0x27] = '|', [0x28] = '>', [0x29] = '~', [0x2B] = '|', [0x33] = ';', [0x34] = ':', [0x35] = '_',
};

// /* Obtiene el ultimo caracter del buffer de entrada, devuelve -1 si no hay */
// char getChar() {
// 	_sti();
// 	char letter;
// 	do {
// 		letter = getKey();
// 	} while (letter == -1 || conversionArray[letter] == -1);

// 	/* Si shift esta presionado y hay mapeo especial, usarlo */
// 	if (isShiftPressed() && shiftConversionArray[letter] != 0) {
// 		return shiftConversionArray[letter];
// 	}

// 	/* Sino, usar conversion normal (mayusculas para letras) */
// 	return (isUpperCase() && letter != -1 && conversionArray[letter] <= 'z' && conversionArray[letter] >= 'a') ?
// 			   conversionArray[letter] - ('a' - 'A') :
// 		   (letter != -1) ? conversionArray[letter] :
// 							letter;
// }

char getChar() {
	_sti();
	char letter;
	do {
		letter = getKey();
	} while (letter == -1 || conversionArray[letter] == -1);
	char c;

	/* Si shift esta presionado y hay mapeo especial, usarlo */
	if (isShiftPressed() && shiftConversionArray[letter] != 0)
		c = shiftConversionArray[letter];
	else if (isUpperCase() && letter != -1 && conversionArray[letter] <= 'z' && conversionArray[letter] >= 'a')
		c = conversionArray[letter] - ('a' - 'A');
	else
		c = (letter != -1) ? conversionArray[letter] : letter;

	/* Detectar Ctrl+D (EOF) */
	if (isCtrlPressed() && c == 'd') {
		return -1; // señal de EOF
	}

	return c;
}

char getcharNonLoop() {
	_sti();
	char letter = getKey();

	/* Si shift esta presionado y hay mapeo especial, usarlo */
	if (isShiftPressed() && shiftConversionArray[letter] != 0) {
		return shiftConversionArray[letter];
	}

	/* Sino, usar conversion normal */
	return (isUpperCase() && letter != -1 && conversionArray[letter] <= 'z' && conversionArray[letter] >= 'a') ?
			   conversionArray[letter] - ('a' - 'A') :
		   (letter != -1) ? conversionArray[letter] :
							letter;
}

/* Muestra un caracter en la salida estandar */
void putChar(char character, int color, int background) {
	/* En modo texto VGA, color y fondo se ignoran (usar vd_set_color si es necesario) */
	vd_draw_char(character);
}

/* Toma un string de hasta longitud len de la entrada estandar */
/* Terminara siendo la syscall read */
int read_keyboard(char *buffer, int len) {
	if (buffer == NULL || len <= 0) {
		return 0;
	}

	int read_count = 0;
	for (int i = 0; i < len; i++) {
		char value = getChar();
		if (value == -1) {
			return 0;
		}
		buffer[i] = value;
		read_count++;
	}
	return read_count;
}

void write(const char *string, int len, int color, int background) {
	if (string == NULL || len <= 0) {
		return;
	}

	uint8_t previous_color = vd_get_color();
	uint8_t fg = (uint8_t) (color & 0x0F);
	uint8_t bg = (uint8_t) (background & 0x0F);
	uint8_t attribute = (uint8_t) ((bg << 4) | fg);
	vd_set_color(attribute);

	for (int i = 0; i < len; i++) {
		vd_draw_char(string[i]);
	}

	vd_set_color(previous_color);
}
