#include "include/test_util.h"
#include "../include/stinUser.h"
#include "include/syscall.h"

/* Generador de numeros aleatorios - Multiply-with-carry */
static uint32_t m_z = 362436069;
static uint32_t m_w = 521288629;

/* Genera un numero aleatorio de 32 bits */
uint32_t GetUint() {
	m_z = 36969 * (m_z & 65535) + (m_z >> 16);
	m_w = 18000 * (m_w & 65535) + (m_w >> 16);
	return (m_z << 16) + m_w;
}

/* Genera un numero aleatorio uniforme entre 0 y max-1 */
uint32_t GetUniform(uint32_t max) {
	if (max == 0) {
		return 0;
	}
	uint32_t u = GetUint();
	/* Evitar punto flotante: usar modulo */
	return u % max;
}

/* Verifica que un bloque de memoria contenga el valor especificado */
uint8_t memcheck(void *start, uint8_t value, uint32_t size) {
	uint8_t *p = (uint8_t *) start;
	uint32_t i;

	for (i = 0; i < size; i++, p++)
		if (*p != value)
			return 0;

	return 1;
}

/* Convierte un string a entero (String to Integer) */
int64_t satoi(char *str) {
	uint64_t i = 0;
	int64_t res = 0;
	int8_t sign = 1;

	if (!str)
		return 0;

	if (str[i] == '-') {
		i++;
		sign = -1;
	}

	for (; str[i] != '\0'; ++i) {
		if (str[i] < '0' || str[i] > '9')
			return 0;
		res = res * 10 + str[i] - '0';
	}

	return res * sign;
}

/* Espera activa (busy wait) - para sincronizacion en tests */
void bussy_wait(uint64_t n) {
	uint64_t i;
	for (i = 0; i < n; i++)
		;
}

/* Loop infinito - para tests de procesos */
void endless_loop() {
	while (1)
		;
}

/* Loop infinito que imprime PID - para tests de procesos */
void endless_loop_print(uint64_t wait) {
	int64_t pid = my_getpid();

	while (1) {
		print("PID: ");
		printBase(pid, 10);
		print(" ");
		bussy_wait(wait);
	}
}
