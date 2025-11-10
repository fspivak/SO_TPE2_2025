#include "include/stinUser.h"
#include "include/libasmUser.h"
#include <stdarg.h>
#include <stddef.h>

static char buffer[64] = {'0'};
static char format_buffer[256] = {'0'};
#define PRINT_SEM_NAME "print_sem"
static int print_sem_initialized = 0;

static uint32_t uintToBase(uint64_t value, char *buffer, uint32_t base);

static void init_print_semaphore() {
	if (!print_sem_initialized) {
		if (sem_open(PRINT_SEM_NAME, 1) >= 0) {
			print_sem_initialized = 1;
		}
	}
}

void print_format(const char *format, ...) {
	va_list args;
	va_start(args, format);

	char *output = format_buffer;
	int output_pos = 0;
	const char *fmt = format;

	init_print_semaphore();
	if (print_sem_initialized) {
		sem_wait(PRINT_SEM_NAME);
	}

	while (*fmt != '\0' && output_pos < 255) {
		if (*fmt != '%') {
			if (output_pos < 255) {
				output[output_pos++] = *fmt;
			}
			fmt++;
			continue;
		}

		fmt++;
		if (*fmt == '\0') {
			if (output_pos < 255) {
				output[output_pos++] = '%';
			}
			break;
		}

		int long_flag = 0;
		while (*fmt == 'l') {
			long_flag++;
			fmt++;
		}

		char specifier = *fmt;
		if (specifier == '\0') {
			break;
		}

		switch (specifier) {
			case 'd':
			case 'i': {
				int64_t value = (long_flag > 0) ? va_arg(args, long long) : va_arg(args, int);
				if (value < 0) {
					if (output_pos < 255) {
						output[output_pos++] = '-';
					}
					uint64_t magnitude = (uint64_t) (-(value + 1)) + 1;
					uintToBase(magnitude, buffer, 10);
				}
				else {
					uintToBase((uint64_t) value, buffer, 10);
				}
				for (int i = 0; buffer[i] != '\0' && output_pos < 255; i++) {
					output[output_pos++] = buffer[i];
				}
				break;
			}
			case 'u': {
				uint64_t value = (long_flag > 0) ? va_arg(args, unsigned long long) : va_arg(args, unsigned int);
				uintToBase(value, buffer, 10);
				for (int i = 0; buffer[i] != '\0' && output_pos < 255; i++) {
					output[output_pos++] = buffer[i];
				}
				break;
			}
			case 'x':
			case 'X': {
				uint64_t value = (long_flag > 0) ? va_arg(args, unsigned long long) : va_arg(args, unsigned int);
				uintToBase(value, buffer, 16);
				for (int i = 0; buffer[i] != '\0' && output_pos < 255; i++) {
					char ch = buffer[i];
					if (specifier == 'x' && ch >= 'A' && ch <= 'F') {
						ch = (char) (ch - 'A' + 'a');
					}
					output[output_pos++] = ch;
				}
				break;
			}
			case 'p': {
				void *ptr = va_arg(args, void *);
				uint64_t value = (uint64_t) ptr;
				if (output_pos < 255) {
					output[output_pos++] = '0';
				}
				if (output_pos < 255) {
					output[output_pos++] = 'x';
				}
				uintToBase(value, buffer, 16);
				for (int i = 0; buffer[i] != '\0' && output_pos < 255; i++) {
					char ch = buffer[i];
					if (ch >= 'A' && ch <= 'F') {
						ch = (char) (ch - 'A' + 'a');
					}
					output[output_pos++] = ch;
				}
				break;
			}
			case 's': {
				char *str = va_arg(args, char *);
				if (str == NULL) {
					str = "(null)";
				}
				for (int i = 0; str[i] != '\0' && output_pos < 255; i++) {
					output[output_pos++] = str[i];
				}
				break;
			}
			case 'c': {
				char c = (char) va_arg(args, int);
				if (output_pos < 255) {
					output[output_pos++] = c;
				}
				break;
			}
			case '%': {
				if (output_pos < 255) {
					output[output_pos++] = '%';
				}
				break;
			}
			default: {
				if (output_pos < 255) {
					output[output_pos++] = '%';
				}
				for (int i = 0; i < long_flag && output_pos < 255; i++) {
					output[output_pos++] = 'l';
				}
				if (output_pos < 255) {
					output[output_pos++] = specifier;
				}
				break;
			}
		}

		fmt++;
	}
	output[output_pos] = '\0';

	int len = 0;
	while (output[len] != '\0' && len < 255) {
		len++;
	}

	if (print_sem_initialized) {
		write(1, output, len, 0x00ffffff, 0);
		sem_post(PRINT_SEM_NAME);
	}
	else {
		write(1, output, len, 0x00ffffff, 0);
	}

	va_end(args);
}

void printColor(char *string, int color, int bg) {
	int i;
	for (i = 0; string[i] != '\0'; i++) {
		putCharColor(string[i], color, bg);
	}
}

char getchar() {
	char caracter;
	read(0, &caracter, 1);
	return caracter;
}

void putchar(char carac) {
	putCharColor(carac, 0x00ffffff, 0);
}

void putCharColor(char carac, int color, int bg) {
	write(1, &carac, 1, color, bg);
}

/* Convierte un entero sin signo a string en la base especificada */
static uint32_t uintToBase(uint64_t value, char *buffer, uint32_t base) {
	char *p = buffer;
	char *p1, *p2;
	uint32_t digits = 0;

	/* Calcula caracteres para cada digito */
	do {
		uint32_t remainder = value % base;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
		digits++;
	} while (value /= base);

	/* Termina el string con null */
	*p = 0;

	/* Invierte el string en el buffer */
	p1 = buffer;
	p2 = p - 1;
	while (p1 < p2) {
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}

	return digits;
}

void zoomIn() {
	zoom(1);
}
void zoomOut() {
	zoom(-1);
}

void imprimirRegistros() {
	impRegs();
}

void sleepUser(int segs) {
	sleep(segs);
}

uint64_t getMS() {
	uint64_t milis;
	getMiliSecs(&milis);
	return milis;
}

void clock() {
	char *str = "00:00:00";
	getClock(str);
	printClock(str);
}

void printClock(char *str) {
	print_format("Hour: %s\n", str);
}

char getcharNonLoop() {
	char charac;
	getcharNL(&charac);
	return charac;
}

/* Genera un numero aleatorio usando generador congruencial lineal */
unsigned int generarNumeroAleatorio(uint64_t *seed) {
	const unsigned long a = 1664525;	/* Multiplicador */
	const unsigned long c = 1013904223; /* Incremento */
	const unsigned long m = 4294967296; /* 2^32 */

	/* Genera el siguiente numero aleatorio */
	*seed = (*seed * a + c) % m;
	return *seed;
}

void sound(int index) {
	playSound(index);
}

int numeroAleatorioEntre(int min, int max, uint64_t *seed) {
	unsigned int numero = generarNumeroAleatorio(seed);
	return (numero % (max - min + 1)) + min;
}
