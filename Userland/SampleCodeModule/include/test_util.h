#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdint.h>

/* Generadores de numeros aleatorios */
uint32_t GetUint();
uint32_t GetUniform(uint32_t max);

/* Utilidades de memoria */
uint8_t memcheck(void *start, uint8_t value, uint32_t size);


/* Conversion de strings */
int64_t satoi(char *str);

/* Utilidades de sincronizacion y procesos (para tests futuros) */
void bussy_wait(uint64_t n);
void endless_loop();
void endless_loop_print(uint64_t wait);

/* Test principal */
void test_mm(uint64_t max_memory);

#endif

