#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdint.h>

/* Tests de Memory Manager */
/**
 * @brief Test principal del Memory Manager con ajuste dinamico
 * @param argc Numero de argumentos (debe ser 1)
 * @param argv Array de argumentos [0] = tamaño de memoria en bytes
 * @return 1 si el test termina correctamente, -1 si hay error
 */
uint64_t test_mm(uint64_t argc, char *argv[]);

/* Generadores de numeros aleatorios */
/**
 * @brief Genera un numero aleatorio de 32 bits usando Multiply-with-carry
 * @return Numero aleatorio de 32 bits
 */
uint32_t GetUint();

/**
 * @brief Genera un numero aleatorio uniforme entre 0 y max-1
 * @param max Valor maximo (exclusivo) del rango
 * @return Numero aleatorio entre 0 y max-1
 */
uint32_t GetUniform(uint32_t max);

/**
 * @brief Verifica que un bloque de memoria contenga el valor especificado
 * @param start Puntero al inicio del bloque de memoria
 * @param value Valor esperado en cada byte
 * @param size Tamaño del bloque en bytes
 * @return 1 si todos los bytes tienen el valor correcto, 0 si hay error
 */
uint8_t memcheck(void *start, uint8_t value, uint32_t size);

/* Conversion de strings */
/**
 * @brief Convierte un string a entero (String to Integer)
 * @param str String a convertir
 * @return Valor entero convertido o 0 si hay error
 */
int64_t satoi(char *str);

/* Utilidades de sincronizacion y procesos */
void bussy_wait(uint64_t n);
void endless_loop();
void endless_loop_print(uint64_t wait);

#endif
