#include "include/stdinout.h"
#include "include/interrupts.h"
#include "include/keyboard.h"
#include "include/videoDriver.h"
#include "scheduler/include/process.h"
#include "scheduler/include/scheduler.h"
#include <stddef.h>
#include <string.h>

// Declaracion externa para obtener el proceso foreground
extern process_id_t get_foreground_process(void);

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

	PCB *current_proc = get_current_process();
	int is_terminal = (current_proc != NULL &&
					   (strcmp(current_proc->name, "shell") == 0 || strcmp(current_proc->name, "shellCreated") == 0));

	if (is_terminal) {
		int read_count = 0;
		for (int i = 0; i < len; i++) {
			char value = getChar();
			// Para el terminal, EOF (Ctrl+D) se maneja en terminal.c
			// Retornar 0 para que terminal.c pueda detectarlo y cerrar stdin del proceso foreground
			if (value == -1) {
				// Si ya leimos algo, retornarlo antes de procesar EOF
				if (read_count > 0) {
					return read_count;
				}
				// Si no hay datos, retornar 0 para que terminal.c maneje EOF
				// pero NO marcar stdin_eof del terminal (ya lo manejamos en process_read)
				return 0;
			}
			buffer[i] = value;
			read_count++;

			// CRITICO: El terminal NO debe hacer echo automatico
			// El terminal solo lee del teclado para pasarlo al proceso foreground
			// El echo es responsabilidad del proceso foreground, no del terminal
			// Si el proceso foreground necesita echo, lo hara manualmente o usara echo automatico
			// en la rama de procesos no-terminal (mas abajo)
		}
		return read_count;
	}

	// Para procesos no-terminal, SOLO el proceso foreground puede leer del teclado
	// Esto evita race conditions entre el terminal y otros procesos
	PCB *proc = get_current_process();
	if (proc == NULL) {
		return 0;
	}

	process_id_t current_pid = proc->pid;
	process_id_t fg_pid = get_foreground_process();

	// Si el proceso actual no es foreground, esperar a que lo sea
	// Esto es necesario porque cat puede empezar a leer antes de ser foreground
	if (fg_pid != current_pid) {
		// Esperar a ser foreground (hacer yield hasta que lo seamos)
		// Esto evita que cat termine inmediatamente si no es foreground aun
		int max_wait = 100; // Limitar espera para evitar loop infinito
		while (fg_pid != current_pid && max_wait > 0) {
			force_switch();
			fg_pid = get_foreground_process();
			max_wait--;
		}
		// Si aun no somos foreground despues de esperar, retornar 0
		if (fg_pid != current_pid) {
			return 0;
		}
	}

	// El proceso actual es foreground, puede leer del teclado
	// Leer caracteres disponibles inmediatamente, sin bloquear innecesariamente
	int read_count = 0;

	while (read_count < len) {
		// Verificar que sigamos siendo foreground antes de leer
		fg_pid = get_foreground_process();
		if (fg_pid != current_pid) {
			// Ya no somos foreground, retornar lo que leimos
			return read_count;
		}

		// Intentar leer caracteres disponibles sin bloquear
		char value = -1;
		char letter = getKey();

		if (letter != -1 && conversionArray[letter] != -1) {
			// Hay un caracter valido disponible
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
				value = -1; // señal de EOF
			}
			else {
				value = c;
			}
		}

		if (value == -1) {
			// No hay mas caracteres disponibles en este momento
			// Si ya leimos algo, retornarlo inmediatamente para que el proceso pueda hacer echo
			if (read_count > 0) {
				return read_count;
			}
			// Si no hay caracteres y no hemos leido nada, usar getChar que bloquea
			// pero esto es necesario para esperar a que el usuario escriba
			// getChar() tiene un loop interno que espera hasta que haya un caracter valido
			value = getChar();
			if (value == -1) {
				// EOF: retornar lo que se haya leido hasta ahora
				if (read_count == 0) {
					return 0;
				}
				break;
			}
		}

		// Guardar el caracter leido
		buffer[read_count++] = value;

		// CRITICO: Hacer echo automatico si el proceso tiene stdin_echo activado
		// EXCEPTO para comandos que escriben manualmente (cat cuando stdout == SCREEN)
		// filter ya no escribe manualmente mientras lee, almacena y filtra al final
		// Esto evita doble echo en comandos como cat que escriben manualmente
		// pero permite echo automatico en comandos como wc y filter que no escriben manualmente
		// Cuando filter esta en un pipe, stdin_echo ya esta desactivado, asi que no hara echo automatico
		if (proc != NULL && proc->io_state.stdin_echo) {
			// Verificar si el proceso es cat (que escribe manualmente cuando stdout == SCREEN)
			// filter ya no escribe manualmente, almacena y filtra al final
			// Solo verificar si stdout es SCREEN para evitar problemas con pipes
			int writes_manually = (proc->io_state.stdout_desc.type == IO_SINK_SCREEN && strcmp(proc->name, "cat") == 0);

			// Solo hacer echo automatico si el proceso NO escribe manualmente
			if (!writes_manually) {
				// Hacer echo del caracter directamente a pantalla
				// Usar el mismo color que write_output() usa por defecto (blanco = 0x0F)
				// para mantener consistencia visual entre echo automatico y escritura manual
				uint8_t previous_color = vd_get_color();
				vd_set_color(VGA_COLOR_WHITE); // Blanco (0x0F) - mismo color que write_output por defecto

				// Solo hacer echo de caracteres imprimibles (no EOF, no backspace especial)
				if (value != -1 && value != '\0') {
					// Manejar backspace: retroceder cursor y borrar caracter
					if (value == '\b' || value == 8) {
						vd_draw_char('\b');
					}
					// Manejar newline: mostrar newline normalmente
					else if (value == '\n') {
						vd_draw_char('\n');
					}
					// Caracteres imprimibles normales
					else if (value >= 32 && value <= 126) {
						vd_draw_char(value);
					}
				}

				// Restaurar el color anterior
				vd_set_color(previous_color);
			}
		}

		// CRITICO: Retornar inmediatamente cuando leemos un caracter para permitir echo en tiempo real
		// Esto es especialmente importante para cat y filter que leen carácter por carácter
		// y necesitan escribir inmediatamente para dar feedback visual
		// Retornar inmediatamente para que el proceso pueda hacer echo antes de leer mas caracteres
		return read_count;
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

	// Escribir caracter por caracter inmediatamente
	// vd_draw_char escribe directamente a la memoria de video (0xB8000)
	// por lo que cada caracter se muestra inmediatamente en pantalla
	for (int i = 0; i < len; i++) {
		vd_draw_char(string[i]);
		// No hay buffering: cada caracter se escribe directamente a VGA
		// y se muestra inmediatamente en pantalla
	}

	vd_set_color(previous_color);
}
