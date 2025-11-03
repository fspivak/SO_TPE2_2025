#include <stdint.h>

/**
 * @brief Inicia el terminal interactivo del sistema
 */
void terminal();

/**
 * @brief Limpia una cantidad especifica de caracteres en pantalla
 * @param ammount Cantidad de caracteres a limpiar
 */
void clean(int ammount);

/**
 * @brief Refresca toda la pantalla
 */
void refreshScreen();

/* Process management functions */

/**
 * @brief Ejecuta el test de procesos
 * @param args Argumentos opcionales para el test (cantidad de procesos)
 */
void run_test_process(char *args);

/**
 * @brief Ejecuta el test del Memory Manager
 * @param args Argumentos opcionales para el test (tamaño de memoria en bytes)
 */
void run_test_mm(char *args);

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
 * @brief llama al entry para usar el clock como un proceso
 */
void callClock();

void create_new_shell();

void exit_shell();

////TODO: BORRAR ESTE TEST/////////
void run_test_jero(char *args);