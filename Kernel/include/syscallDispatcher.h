#ifndef SYSCALL_DISPATCHER_H
#define SYSCALL_DISPATCHER_H

#include "../memory-manager/include/memory_manager.h"
#include "../scheduler/include/process.h"
#include "../scheduler/include/semaphore.h"
#include "process_io_config.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum { STDIN = 0, STDOUT = 1, STDERR = 2 } FDS;

/**
 * @brief Atiende una llamada a syscall y ejecuta la rutina correspondiente
 * @param rax Identificador de la syscall solicitada
 * @param ... Argumentos variadicos de la syscall
 * @return Valor de retorno especifico de cada syscall
 */
uint64_t syscallDispatcher(uint64_t rax, ...);

/**
 * @brief Escribe datos en un descriptor de salida
 * @details Esta syscall determina automaticamente si debe escribir a la pantalla o a un pipe
 * basandose en la configuracion de IO del proceso actual. Los procesos no necesitan conocer
 * el destino real de sus escrituras, haciendo la redireccion completamente transparente.
 * @param fd Descriptor de salida (STDOUT o STDERR)
 * @param buf Buffer con los datos a escribir
 * @param count Cantidad de bytes a escribir
 * @param color Color de primer plano
 * @param background Color de fondo
 */
void sys_write(FDS fd, const char *buf, size_t count, size_t color, size_t background);

/**
 * @brief Lee datos desde un descriptor de entrada
 * @details Esta syscall determina automaticamente si debe leer del teclado o de un pipe
 * basandose en la configuracion de IO del proceso actual. Los procesos no necesitan conocer
 * el origen real de sus lecturas, haciendo la redireccion completamente transparente.
 * La lectura desde pipes es bloqueante si no hay datos disponibles.
 * @param fd Descriptor de entrada (STDIN)
 * @param buffer Buffer de destino
 * @param count Cantidad de bytes solicitados
 * @return Cantidad de bytes leidos, 0 si EOF, -1 en error
 */
int sys_read(FDS fd, char *buffer, size_t count);

/**
 * @brief Lee datos desde la entrada estandar del proceso (stdin)
 * @details Esta syscall lee automaticamente desde stdin sin requerir file descriptor.
 * Determina automaticamente si debe leer del teclado o de un pipe basandose en la
 * configuracion de IO del proceso actual. Los comandos no necesitan conocer file descriptors.
 * @param buffer Buffer de destino
 * @param count Cantidad de bytes solicitados
 * @return Cantidad de bytes leidos, 0 si EOF, -1 en error
 */
int sys_read_input(char *buffer, size_t count);

/**
 * @brief Escribe datos a la salida estandar del proceso (stdout)
 * @details Esta syscall escribe automaticamente a stdout sin requerir file descriptor.
 * Determina automaticamente si debe escribir a la pantalla o a un pipe basandose en la
 * configuracion de IO del proceso actual. Los comandos no necesitan conocer file descriptors.
 * @param buf Buffer con los datos a escribir
 * @param count Cantidad de bytes a escribir
 * @param color Color de primer plano
 * @param background Color de fondo
 */
void sys_write_output(const char *buf, size_t count, size_t color, size_t background);

/**
 * @brief Suspende el proceso actual por una cantidad de segundos
 * @param seconds Segundos a esperar
 */
void sys_sleep(int seconds);

/**
 * @brief Cambia el zoom de la pantalla
 * @param zoom Nivel de zoom solicitado
 */
void sys_zoom(int zoom);

/**
 * @brief Dibuja un pixel en la pantalla
 * @param color Color del pixel
 * @param x Coordenada horizontal
 * @param y Coordenada vertical
 */
void sys_draw(int color, int x, int y);

/**
 * @brief Obtiene las dimensiones de la pantalla
 * @param width Parametro de salida para el ancho
 * @param height Parametro de salida para la altura
 */
void sys_screenDetails(int *width, int *height);

/**
 * @brief Actualiza la posicion del cursor
 * @param x Coordenada horizontal
 * @param y Coordenada vertical
 */
void sys_setCursor(int x, int y);

/**
 * @brief Limpia el contenido de la pantalla
 */
void sys_clear_screen();

/**
 * @brief Obtiene la hora actual como cadena
 * @param str Buffer de salida
 */
void sys_getClock(char *str);

/**
 * @brief Obtiene los milisegundos transcurridos desde el arranque
 * @param milis Parametro de salida con los milisegundos
 */
void sys_getMilis(uint64_t *milis);

/**
 * @brief Lee un caracter del teclado sin esperar nueva linea
 * @param charac Parametro de salida para el caracter
 */
void sys_getcharNL(char *charac);

/**
 * @brief Reproduce un sonido por indice
 * @param index Identificador del sonido
 */
void sys_playSound(int index);

/**
 * @brief Imprime los registros almacenados
 */
void sys_impRegs();

/* Memory Manager syscalls */
/**
 * @brief Reserva memoria dinamica
 * @param size Cantidad de bytes solicitados
 * @return Puntero a la memoria reservada o NULL si falla
 */
void *sys_malloc(uint64_t size);

/**
 * @brief Libera memoria previamente reservada
 * @param ptr Puntero a liberar
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_free(void *ptr);

/**
 * @brief Obtiene el estado del administrador de memoria
 * @param state Buffer de salida con la informacion de memoria
 */
void sys_mem_status(HeapState *state);

/* Process syscalls */
/**
 * @brief Crea un nuevo proceso
 * @param name Nombre del proceso
 * @param entry_point Funcion de entrada
 * @param argc Cantidad de argumentos
 * @param argv Argumentos del proceso
 * @param priority Prioridad solicitada
 * @return PID del proceso creado o -1 si falla
 */
process_id_t sys_create_process(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
								uint8_t priority);

/**
 * @brief Crea un proceso y lo ejecuta en foreground
 * @param name Nombre del proceso
 * @param entry_point Funcion de entrada
 * @param argc Cantidad de argumentos
 * @param argv Argumentos del proceso
 * @param priority Prioridad solicitada
 * @return PID del proceso creado o -1 si falla
 */
process_id_t sys_create_process_foreground(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
										   uint8_t priority);

/**
 * @brief Crea un proceso con configuracion de IO personalizada
 * @param name Nombre del proceso
 * @param entry_point Funcion de entrada
 * @param argc Cantidad de argumentos
 * @param argv Argumentos del proceso
 * @param priority Prioridad solicitada
 * @param config Configuracion de IO requerida
 * @return PID del proceso creado o -1 si falla
 */
process_id_t sys_create_process_with_io(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
										uint8_t priority, const process_io_config_t *config);

/**
 * @brief Crea un proceso en foreground con configuracion de IO
 * @param name Nombre del proceso
 * @param entry_point Funcion de entrada
 * @param argc Cantidad de argumentos
 * @param argv Argumentos del proceso
 * @param priority Prioridad solicitada
 * @param config Configuracion de IO requerida
 * @return PID del proceso creado o -1 si falla
 */
process_id_t sys_create_process_foreground_with_io(const char *name, void (*entry_point)(int, char **), int argc,
												   char **argv, uint8_t priority, const process_io_config_t *config);

/**
 * @brief Obtiene el PID del proceso actual
 * @return PID del proceso en ejecucion
 */
process_id_t sys_getpid();

/**
 * @brief Obtiene el tipo de stdin del proceso actual
 * @return PROCESS_IO_STDIN_KEYBOARD o PROCESS_IO_STDIN_PIPE
 */
uint32_t sys_get_stdin_type();

/**
 * @brief Obtiene el tipo de stdout del proceso actual
 * @return PROCESS_IO_STDOUT_SCREEN o PROCESS_IO_STDOUT_PIPE
 */
uint32_t sys_get_stdout_type();

/**
 * @brief Cierra el stdin del proceso actual (marca EOF)
 * @return 0 si exito, -1 si error
 */
int sys_close_stdin(void);

/**
 * @brief Cierra el stdin de un proceso especifico (marca EOF)
 * @param pid PID del proceso cuyo stdin se desea cerrar
 * @return 0 si exito, -1 si error
 */
int sys_close_stdin_pid(process_id_t pid);

/**
 * @brief Obtiene el PID del proceso foreground
 * @return PID del proceso foreground o -1 si no hay
 */
process_id_t sys_get_foreground_process(void);

/**
 * @brief Finaliza un proceso
 * @param pid PID del proceso a finalizar
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_kill(process_id_t pid);

/**
 * @brief Bloquea un proceso
 * @param pid PID del proceso a bloquear
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_block(process_id_t pid);

/**
 * @brief Desbloquea un proceso
 * @param pid PID del proceso a desbloquear
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_unblock(process_id_t pid);

/**
 * @brief Cambia la prioridad de un proceso
 * @param pid PID del proceso
 * @param new_priority Prioridad solicitada
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_nice(process_id_t pid, uint8_t new_priority);

/**
 * @brief Cede la CPU de forma voluntaria
 */
void sys_yield();

/**
 * @brief Obtiene informacion de procesos activos
 * @param buffer Buffer de salida
 * @param max_processes Cantidad maxima a obtener
 * @return Cantidad de procesos copiados o -1 si ocurre un error
 */
int sys_ps(ProcessInfo *buffer, int max_processes);

/**
 * @brief Termina el proceso actual
 */
void sys_exit();

/**
 * @brief Espera a que un proceso termine
 * @param pid PID del proceso a esperar
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_waitpid(process_id_t pid);

/* Semaphore syscalls */
/**
 * @brief Abre o crea un semaforo
 * @param name Nombre del semaforo
 * @param initial_value Valor inicial
 * @return Identificador del semaforo o -1 si ocurre un error
 */
int sys_sem_open(const char *name, uint32_t initial_value);

/**
 * @brief Decrementa el semaforo o bloquea el proceso
 * @param name Nombre del semaforo
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_sem_wait(const char *name);

/**
 * @brief Incrementa el semaforo o desbloquea un proceso
 * @param name Nombre del semaforo
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_sem_post(const char *name);

/**
 * @brief Cierra un semaforo
 * @param name Nombre del semaforo
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_sem_close(const char *name);

/* Foreground helpers */
/**
 * @brief Establece un proceso como propietario del foreground
 * @param pid PID del proceso a marcar
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_set_foreground(process_id_t pid);

/**
 * @brief Elimina el foreground del proceso indicado
 * @param pid PID del proceso que libera el foreground
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int sys_clear_foreground(process_id_t pid);

#endif /* SYSCALL_DISPATCHER_H */