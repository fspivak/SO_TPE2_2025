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

/**
 * @brief Ejecuta el test de prioridades
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void test_prio_cmd(int argc, char **argv);

/**
 * @brief Crea un proceso que ejecuta un loop infinito
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void loop_cmd(int argc, char **argv);

/**
 * @brief Mata un proceso por su PID
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void kill_cmd(int argc, char **argv);

/**
 * @brief Cambia la prioridad de un proceso
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void nice_cmd(int argc, char **argv);

/**
 * @brief Bloquea un proceso por su PID
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void block_cmd(int argc, char **argv);

/////////////TODO: fletar este test////////////////
void test_pipe_cmd(int argc, char **argv);

/**
 * @brief Valida un argumento de PID y verifica que existe
 * @param cmd_name Nombre del comando que llama a esta funcion
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 * @param arg_index Indice del argumento a validar
 * @return PID validado si es correcto, 0 si hay error
 */
int validate_pid_arg(const char *cmd_name, int argc, char **argv, int arg_index);

/**
 * @brief Valida un argumento de prioridad
 * @param cmd_name Nombre del comando que llama a esta funcion
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 * @param arg_index Indice del argumento a validar
 * @return Prioridad validada (0-255) si es correcta, -1 si hay error
 */
int validate_priority_arg(const char *cmd_name, int argc, char **argv, int arg_index);

/**
 * @brief Valida que un argumento sea un entero no negativo
 * @param cmd_name Nombre del comando que llama a esta funcion
 * @param arg_name Nombre del argumento para mensajes de error
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 * @param arg_index Indice del argumento a validar
 * @return Valor entero no negativo si es correcto, -1 si hay error
 */
int validate_non_negative_int(const char *cmd_name, const char *arg_name, int argc, char **argv, int arg_index);

/**
 * @brief Valida el resultado de create_process y muestra error si falla
 * @param cmd_name Nombre del comando que intento crear el proceso
 * @param pid PID retornado por create_process
 * @return 1 si el proceso se creo correctamente, 0 si hubo error
 */
int validate_create_process_error(const char *cmd_name, int pid);

/**
 * @brief printea en pantalla lo ingresado en stdin
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void cat_cmd(int argc, char **argv);

/**
 * @brief cuante las lineas y caracteres
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void wc_cmd(int argc, char **argv);

/**
 * @brief filtra lineas
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void filter_cmd(int argc, char **argv);

/**
 * @brief implementa los comandos pipeados
 * @param input Input de la pipe
 */
void pipes_cmd(char *input);

/**
 * @brief encuentra la funcion de un comando
 * @param cmd Nombre del comando
 * @return Puntero a la funcion del comando
 */
void *find_function(char *cmd);

#endif // COMMANDS_H
