#include "include/exceptions.h"
#include "include/time.h"
#include "include/videoDriver.h"
#include <stdint.h>

#define ZERO_EXCEPTION_ID 0
#define INVALID_OPCODE_ID 6

/* Declaraciones de funciones internas */
static void zero_division();
static uint32_t uintToBase(uint64_t value, char *buffer, uint32_t base);
void PrintBase(uint64_t value, uint32_t base);
void invalidOpcode();

typedef int (*EntryPoint)();

static void *const sampleCodeModuleAddress = (void *) 0x400000;

void exceptionDispatcher(int exception) {
	if (exception == ZERO_EXCEPTION_ID)
		zero_division();
	else if (exception == INVALID_OPCODE_ID)
		invalidOpcode();
}

/* Maneja la excepcion de division por cero */
static void zero_division() {
	vd_print("\nDividiste por 0\nRetornando a terminal\n");
	sleep(3 * 18);
}

/* Maneja la excepcion de opcode invalido */
void invalidOpcode() {
	vd_print("\nOpcode invalido\nRetornando a terminal\n");
	sleep(3 * 18);
}

/* Imprime los valores de todos los registros en formato hexadecimal */
void printRegistros(uint64_t *registros) {
	vd_print("RAX: ");
	PrintBase(registros[0], 16);
	vd_print("\n");

	vd_print("RBX: ");
	PrintBase(registros[1], 16);
	vd_print("\n");

	vd_print("RCX: ");
	PrintBase(registros[2], 16);
	vd_print("\n");

	vd_print("RDX: ");
	PrintBase(registros[3], 16);
	vd_print("\n");

	vd_print("RSI: ");
	PrintBase(registros[4], 16);
	vd_print("\n");

	vd_print("RDI: ");
	PrintBase(registros[5], 16);
	vd_print("\n");

	vd_print("RBP: ");
	PrintBase(registros[6], 16);
	vd_print("\n");

	vd_print("RSP: ");
	PrintBase(registros[7], 16);
	vd_print("\n");

	vd_print("R8: ");
	PrintBase(registros[8], 16);
	vd_print("\n");

	vd_print("R9: ");
	PrintBase(registros[9], 16);
	vd_print("\n");

	vd_print("RIP: ");
	PrintBase(registros[10], 16);
	vd_print("\n");

	vd_print("CS: ");
	PrintBase(registros[11], 16);
	vd_print("\n");

	vd_print("RFLAGS: ");
	PrintBase(registros[12], 16);
	vd_print("\n");
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

/* Imprime un valor en la base especificada */
void PrintBase(uint64_t value, uint32_t base) {
	char buffer[100];
	uintToBase(value, buffer, base);
	vd_print(buffer);
}