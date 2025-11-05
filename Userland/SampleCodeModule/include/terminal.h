#include <stdint.h>

/**
 * @brief Inicia el terminal interactivo del sistema
 */
void terminal();

/* Process management functions */

/* Utility functions */

/**
 * @brief Convierte un entero a string
 * @param value El valor a convertir
 * @param buffer Buffer donde guardar el string
 */
void intToString(int value, char *buffer);

/**
 * @brief Verifica si un string comienza con otro
 * @param str El string principal
 * @param prefix El prefijo a buscar
 * @return 1 si comienza con el prefijo, 0 si no
 */
int startsWith(const char *str, const char *prefix);

/**
 * @brief Llama al entry para usar el clock como un proceso
 */
void callClock();

/**
 * @brief Crea una nueva shell a partir de la actual
 */
void create_new_shell();

/**
 * @brief Sale y mata la terminal actual
 */
void exit_shell();
