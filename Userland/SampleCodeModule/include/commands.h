#ifndef COMMANDS_H
#define COMMANDS_H

#include "../../../Kernel/include/process_io_config.h"

/**
 * @brief Configura si el comando actual se ejecuta en background
 * @param enabled 1 si se ejecuta en background, 0 en caso contrario
 */
void command_set_background_mode(int enabled);

/**
 * @brief Indica si el comando actual se esta ejecutando en background
 * @return 1 si el comando corre en background, 0 de lo contrario
 */
int command_is_background_mode(void);

/**
 * @brief Resetea el estado global de ejecucion en background
 */
void command_reset_background_mode(void);

/**
 * @brief Obtiene (y limpia) una notificacion pendiente de proceso en background
 * @param pid Salida opcional para el PID del proceso
 * @param name Salida opcional para el nombre del proceso
 * @return 1 si existia notificacion, 0 en caso contrario
 */
int command_pop_background_notification(int *pid, const char **name);

/**
 * @brief Maneja la relacion foreground/waitpid para un proceso hijo
 * @param pid PID del proceso hijo
 * @param name Nombre del comando (para mensajes)
 */
void command_handle_child_process(int pid, const char *name);

/**
 * @brief Crea un proceso considerando si debe arrancar en foreground o background
 * @param name Nombre del proceso
 * @param entry Funcion de entrada del proceso
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 * @param priority Prioridad solicitada
 * @param io_config Configuracion de IO (NULL para heredar)
 * @return PID del proceso creado o -1 si hay error
 */
int command_spawn_process(const char *name, void (*entry)(int, char **), int argc, char **argv, int priority,
						  const process_io_config_t *io_config);

/**
 * @brief Muestra la ayuda del terminal con comandos disponibles
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void help_cmd(int argc, char **argv);

/**
 * @brief Entrada principal para ejecutar help en un proceso interactivo
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void help_main(int argc, char **argv);

/**
 * @brief Muestra un manual de ayuda para ciertos comandos disponibles
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void man_cmd(int argc, char **argv);

/**
 * @brief Entrada principal para ejecutar man en un proceso interactivo
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void man_main(int argc, char **argv);

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
 * @brief Entrada principal para ejecutar ps en un proceso
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void ps_main(int argc, char **argv);

/**
 * @brief Muestra el estado de la memoria
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void mem_cmd(int argc, char **argv);

/**
 * @brief Entrada principal para ejecutar mem en un proceso interactivo
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void mem_main(int argc, char **argv);

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
 * @brief Crea un proceso que imprime su PID periodicamente
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
 * @brief Entrada principal para ejecutar cat en un proceso interactivo
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void cat_main(int argc, char **argv);

/**
 * @brief Cuenta las lineas y caracteres del input recibido desde stdin
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void wc_cmd(int argc, char **argv);

/**
 * @brief Entrada principal para ejecutar wc en un proceso interactivo
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void wc_main(int argc, char **argv);

/**
 * @brief Filtra las vocales del input recibido desde stdin
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void filter_cmd(int argc, char **argv);

/**
 * @brief Entrada principal para ejecutar filter en un proceso interactivo
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void filter_main(int argc, char **argv);

/**
 * @brief implementa los comandos pipeados
 * @param input Input de la pipe
 */
void pipes_cmd(char *input);

/**
 * @brief Crea un proceso que imprime su PID periodicamente
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 */
void mvar_cmd(int argc, char **argv);

#endif // COMMANDS_H
