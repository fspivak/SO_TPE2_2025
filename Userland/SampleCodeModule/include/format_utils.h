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

/**
 * @brief Convierte un entero a string
 * @param value Valor entero a convertir
 * @param buffer Buffer donde guardar el string resultante
 */
void intToString(int value, char *buffer);

/**
 * @brief Verifica si un string comienza con un prefijo
 * @param str String a verificar
 * @param prefix Prefijo a buscar
 * @return 1 si comienza con el prefijo, 0 si no
 */
int startsWith(const char *str, const char *prefix);

/**
 * @brief Ejecuta un comando con argumentos parseados
 * @param buffer Buffer con el comando completo
 * @param command_prefix Prefijo del comando
 * @param prefix_len Longitud del prefijo
 * @param cmd_func Funcion del comando a ejecutar
 */
void execute_command_with_args(const char *buffer, const char *command_prefix, int prefix_len,
							   void (*cmd_func)(int, char **));

#endif // FORMAT_UTILS_H
