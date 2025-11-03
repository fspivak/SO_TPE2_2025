#ifndef COMMANDS_H
#define COMMANDS_H

/**
 * @brief Muestra la ayuda del terminal con comandos disponibles
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void help_cmd(int argc, char **argv);

/**
 * @brief Limpia la pantalla
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void clear_cmd(int argc, char **argv);

/**
 * @brief Lista todos los procesos activos en el sistema
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void ps_cmd(int argc, char **argv);

/**
 * @brief Muestra el estado de la memoria
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void mem_cmd(int argc, char **argv);

/**
 * @brief Ejecuta el test del memory manager
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void test_mm_cmd(int argc, char **argv);

/**
 * @brief Ejecuta el test de procesos
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void test_process_cmd(int argc, char **argv);

/**
 * @brief Ejecuta el test AB (dos procesos alternando A y B)
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void test_ab_cmd(int argc, char **argv);

/**
 * @brief Funcion principal del test del memory manager
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void test_mm_main(int argc, char **argv);

/**
 * @brief Funcion principal del test de procesos
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void test_process_main(int argc, char **argv);

/**
 * @brief Funcion principal del test AB
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void test_ab_main(int argc, char **argv);

/**
 * @brief Muestra el PID del proceso actual
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void getpid_cmd(int argc, char **argv);

/**
 * @brief Ejecuta el test de sincronizacion (semaforos)
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void test_sync_cmd(int argc, char **argv);

/////////////TODO: fletar este test////////////////
void test_pipe_cmd(int argc, char **argv); // declaración arriba

#endif // COMMANDS_H
