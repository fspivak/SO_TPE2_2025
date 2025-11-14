#ifndef LIBASMUSER_H
#define LIBASMUSER_H

#include "../../../Kernel/include/process_io_config.h"
#include <stdint.h>

/**
 * @brief Escribe texto en pantalla con color y fondo especificados
 * @param fd File descriptor (0=stdin, 1=stdout, 2=stderr)
 * @param str String a escribir
 * @param leng Longitud del string
 * @param color Color del texto
 * @param bg Color de fondo
 */
void write(int fd, char *str, int leng, int color, int bg);

/**
 * @brief Lee caracteres desde el teclado (wrapper ASM historico)
 * @param fd File descriptor (0=stdin)
 * @param buffer Buffer donde guardar los caracteres
 * @param count Numero de caracteres a leer
 */
void read(int fd, char *buffer, int count);

/**
 * @brief Invoca la syscall read devolviendo la cantidad de bytes leidos
 * @param fd File descriptor (0=stdin)
 * @param buffer Buffer donde guardar los caracteres
 * @param count Numero de caracteres a leer
 * @return Cantidad de bytes leidos, 0 si EOF, -1 si error
 */
int read_bytes(int fd, char *buffer, int count);

/**
 * @brief Lee desde la entrada estandar del proceso (stdin)
 * @details Esta funcion oculta el file descriptor - los comandos no necesitan
 * conocer que estan leyendo de stdin. La syscall determina automaticamente
 * si debe leer del teclado o de un pipe segun la configuracion del proceso.
 * @param buffer Buffer donde guardar los caracteres
 * @param count Numero de caracteres a leer
 * @return Cantidad de bytes leidos, 0 si EOF, -1 si error
 */
int read_input(char *buffer, int count);

/**
 * @brief Escribe a la salida estandar del proceso (stdout)
 * @details Esta funcion oculta el file descriptor - los comandos no necesitan
 * conocer que estan escribiendo a stdout. La syscall determina automaticamente
 * si debe escribir a la pantalla o a un pipe segun la configuracion del proceso.
 * @param str String a escribir
 * @param len Longitud del string
 * @param color Color del texto
 * @param bg Color de fondo
 */
void write_output(char *str, int len, int color, int bg);

/**
 * @brief Pausa la ejecucion por el numero de segundos especificado
 * @param seg Numero de segundos a esperar
 */
void sleep(int seg);

/**
 * @brief Dibuja un pixel en las coordenadas especificadas
 * @param color Color del pixel
 * @param x Coordenada X
 * @param y Coordenada Y
 */
void draw(int color, int x, int y);

/**
 * @brief Obtiene las dimensiones de la pantalla
 * @param width Puntero donde guardar el ancho
 * @param height Puntero donde guardar la altura
 */
void screenDetails(int *width, int *height);

/**
 * @brief Establece la posicion del cursor
 * @param x Coordenada X del cursor
 * @param y Coordenada Y del cursor
 */
void setCursor(int x, int y);

/**
 * @brief Limpia la pantalla
 */
void clearScreen();

/**
 * @brief Aloca memoria dinamica
 * @param size Tamaño en bytes a alocar
 * @return Puntero a la memoria alocada o NULL si falla
 */
void *malloc(uint64_t size);

/**
 * @brief Libera memoria previamente alocada
 * @param ptr Puntero a la memoria a liberar
 * @return 0 si exitoso, -1 si hay error
 */
int free(void *ptr);

/**
 * @brief Obtiene el estado del memory manager
 * @param state Puntero a estructura donde guardar el estado
 */
void memStatus(void *state);

/**
 * @brief Obtiene la hora actual del sistema
 * @param str Buffer donde guardar la hora como string
 */
void getClock(char *str);

/**
 * @brief Reproduce un sonido
 * @param index Indice del sonido a reproducir
 */
void playSound(int index);

/**
 * @brief Obtiene los milisegundos transcurridos desde el boot
 * @param milis Puntero donde guardar los milisegundos
 */
void getMiliSecs(uint64_t *milis);

/**
 * @brief Lee un caracter del teclado (con nueva linea)
 * @param charac Puntero donde guardar el caracter
 */
void getcharNL(char *charac);

/**
 * @brief Imprime los registros de la CPU
 */
void impRegs();

/**
 * @brief Termina la ejecucion del proceso actual
 */
void exit(void);

/**
 * @brief Genera una excepcion de opcode invalido (para testing)
 */
void rompeOpcode();

/* ===== SYSCALL GENERICA ===== */

/**
 * @brief Syscall generica que permite invocar cualquier syscall del kernel
 * @param id ID de la syscall (ver syscall_ids.h) (rdi)
 * @param arg1 Primer argumento (rsi)
 * @param arg2 Segundo argumento (rdx)
 * @param arg3 Tercer argumento (rcx)
 * @param arg4 Cuarto argumento (r8)
 * @param arg5 Quinto argumento (r9)
 * @param arg6 Sexto argumento (rbp+16) (stack)
 * @return Valor de retorno de la syscall
 */
extern int64_t sys_call(uint64_t id, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5,
						uint64_t arg6);

/* ===== SYSCALLS DE PROCESOS (en syscalls.c) ===== */

/**
 * @brief Crea un nuevo proceso
 * @param name Nombre del proceso
 * @param entry Funcion de entrada del proceso
 * @param argc Numero de argumentos
 * @param argv Array de argumentos
 * @param priority Prioridad del proceso (0-255)
 * @return PID del proceso creado o -1 si hay error
 */
int create_process(const char *name, void (*entry)(int, char **), int argc, char **argv, int priority);

/**
 * @brief Crea un nuevo proceso asegurando la obtencion inmediata del foreground
 * @param name Nombre del proceso
 * @param entry Funcion de entrada del proceso
 * @param argc Numero de argumentos
 * @param argv Array de argumentos
 * @param priority Prioridad del proceso (0-255)
 * @return PID del proceso creado o -1 si hay error
 */
int create_process_foreground(const char *name, void (*entry)(int, char **), int argc, char **argv, int priority);

/**
 * @brief Crea un nuevo proceso con configuracion de IO personalizada
 * @param name Nombre del proceso
 * @param entry Funcion de entrada del proceso
 * @param argc Numero de argumentos
 * @param argv Array de argumentos
 * @param priority Prioridad del proceso (0-255)
 * @param io_config Configuracion de IO (NULL para heredar)
 * @return PID del proceso creado o -1 si hay error
 */
int create_process_with_io(const char *name, void (*entry)(int, char **), int argc, char **argv, int priority,
						   const process_io_config_t *io_config);

/**
 * @brief Crea un nuevo proceso con IO personalizado asegurando foreground
 * @param name Nombre del proceso
 * @param entry Funcion de entrada del proceso
 * @param argc Numero de argumentos
 * @param argv Array de argumentos
 * @param priority Prioridad del proceso (0-255)
 * @param io_config Configuracion de IO (NULL para heredar)
 * @return PID del proceso creado o -1 si hay error
 */
int create_process_foreground_with_io(const char *name, void (*entry)(int, char **), int argc, char **argv,
									  int priority, const process_io_config_t *io_config);

/**
 * @brief Obtiene el PID del proceso actual
 * @return PID del proceso actual
 */
int getpid(void);

/**
 * @brief Obtiene el tipo de stdin del proceso actual
 * @return PROCESS_IO_STDIN_KEYBOARD o PROCESS_IO_STDIN_PIPE
 */
uint32_t get_stdin_type(void);

/**
 * @brief Obtiene el tipo de stdout del proceso actual
 * @return PROCESS_IO_STDOUT_SCREEN o PROCESS_IO_STDOUT_PIPE
 */
uint32_t get_stdout_type(void);

/**
 * @brief Cierra el stdin del proceso actual (marca EOF)
 * @return 0 si exito, -1 si error
 */
int close_stdin(void);

/**
 * @brief Cierra el stdin de un proceso especifico (marca EOF)
 * @param pid PID del proceso cuyo stdin se desea cerrar
 * @return 0 si exito, -1 si error
 */
int close_stdin_pid(int pid);

/**
 * @brief Obtiene el PID del proceso foreground
 * @return PID del proceso foreground o -1 si no hay
 */
int get_foreground_process(void);

/**
 * @brief Termina un proceso y libera sus recursos
 * @param pid PID del proceso a terminar
 * @return 0 si exitoso, -1 si hay error
 */
int kill(int pid);

/**
 * @brief Bloquea un proceso (cambia estado a BLOCKED)
 * @param pid PID del proceso a bloquear
 * @return 0 si exitoso, -1 si hay error
 */
int block(int pid);

/**
 * @brief Desbloquea un proceso (cambia estado a READY)
 * @param pid PID del proceso a desbloquear
 * @return 0 si exitoso, -1 si hay error
 */
int unblock(int pid);

/**
 * @brief Cambia la prioridad de un proceso
 * @param pid PID del proceso
 * @param priority Nueva prioridad (0-255)
 * @return 0 si exitoso, -1 si hay error
 */
int nice(int pid, int priority);

/**
 * @brief Cede voluntariamente el CPU al scheduler
 */
void yield(void);

// Informacion de proceso para userland
typedef struct {
	int pid;
	char name[32];
	uint8_t priority;
	uint64_t stack_base;
	uint64_t rsp;
	char state_name[16];
	uint8_t hasForeground;
} ProcessInfo;

/**
 * @brief Lista procesos activos
 * @param buffer Buffer donde guardar la informacion
 * @param max_processes Numero maximo de procesos a retornar
 * @return Numero de procesos retornados
 */
int ps(void *buffer, int max_processes);

/**
 * @brief Espera a que un proceso hijo termine
 * @param pid PID del proceso hijo a esperar
 * @return 0 si exitoso, -1 si hay error
 */
int waitpid(int pid);

/**
 * @brief Marca un proceso como foreground (terminal)
 * @param pid PID del proceso a marcar (-1 para liberar)
 * @return 0 si exitoso, -1 si hay error
 */
int set_foreground(int pid);

/**
 * @brief Libera el foreground si coincide con el PID indicado
 * @param pid PID del proceso que libera el foreground
 * @return 0 si exitoso, -1 si hay error
 */
int clear_foreground(int pid);

/**
 * @brief Abre o crea un semaforo con el nombre y valor inicial especificados
 * @param name Nombre del semaforo
 * @param initial_value Valor inicial del semaforo
 * @return 0 si exitoso, -1 si hay error
 */
int sem_open(const char *name, uint32_t initial_value);

/**
 * @brief Espera en el semaforo (decrementa el valor o bloquea el proceso)
 * @param name Nombre del semaforo
 * @return 0 si exitoso, -1 si hay error
 */
int sem_wait(const char *name);

/**
 * @brief Libera el semaforo (incrementa el valor o desbloquea un proceso)
 * @param name Nombre del semaforo
 * @return 0 si exitoso, -1 si hay error
 */
int sem_post(const char *name);

/**
 * @brief Cierra el semaforo (decrementa contador de referencias)
 * @param name Nombre del semaforo
 * @return 0 si exitoso, -1 si hay error
 */
int sem_close(const char *name);

/**
 * @brief Termina el proceso actual liberando sus recursos
 */
void exit_process(void);

#endif // LIBASMUSER_H