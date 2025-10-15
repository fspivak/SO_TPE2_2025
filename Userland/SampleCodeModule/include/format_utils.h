#ifndef FORMAT_UTILS_H
#define FORMAT_UTILS_H

#include <stdint.h>

/**
 * @brief Imprime una cadena y la rellena con espacios hasta un ancho dado
 * @param str La cadena a imprimir
 * @param width El ancho total deseado para la columna
 */
void print_padded(const char *str, int width);

/**
 * @brief Imprime un entero y lo rellena con espacios hasta un ancho dado
 * @param value El valor entero a imprimir
 * @param width El ancho total deseado para la columna
 */
void print_int_padded(int value, int width);

/**
 * @brief Imprime un valor hexadecimal con prefijo 0x y padding
 * @param value El valor a imprimir en hexadecimal
 * @param width El ancho total deseado para la columna (incluyendo 0x)
 */
void print_hex_padded(uint64_t value, int width);

#endif // FORMAT_UTILS_H
