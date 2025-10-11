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
 * @brief Lee caracteres desde el teclado
 * @param fd File descriptor (0=stdin)
 * @param buffer Buffer donde guardar los caracteres
 * @param count Numero de caracteres a leer
 */
void read(int fd, char *buffer, int count);

/**
 * @brief Pausa la ejecucion por el numero de segundos especificado
 * @param seg Numero de segundos a esperar
 */
void sleep(int seg);

/**
 * @brief Cambia el zoom de la pantalla
 * @param specifier Nivel de zoom (1-3)
 */
void zoom(int specifier);

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
 * @brief Obtiene el PID del proceso actual
 * @return PID del proceso actual
 */
int getpid(void);

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

/**
 * @brief Lista procesos activos
 * @param buffer Buffer donde guardar la informacion
 * @param max_processes Numero maximo de procesos a retornar
 * @return Numero de procesos retornados
 */
int ps(void *buffer, int max_processes);