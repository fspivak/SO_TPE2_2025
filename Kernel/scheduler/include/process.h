#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "../../include/process_io_config.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_PROCESSES 64
#define STACK_SIZE 4096 // 4KB por process stack
#define MAX_PROCESS_NAME 32
#define MIN_PRIORITY 0	   // mayor prioridad
#define MAX_PRIORITY 255   // menor prioridad
#define CLEANUP_INTERVAL 3 // Limpiar procesos terminados cada N context switches

typedef int process_id_t;

// Estados de proceso
typedef enum {
	READY,	   // Listo para ejecutar
	RUNNING,   // Ejecutando actualmente
	BLOCKED,   // Bloqueado (esperando algo)
	TERMINATED // Terminado (liberar recursos)
} ProcessState;

typedef enum { IO_SOURCE_KEYBOARD, IO_SOURCE_PIPE } IOInputSourceType;

typedef enum { IO_SINK_SCREEN, IO_SINK_PIPE } IOOutputSinkType;

typedef struct {
	IOInputSourceType type;
	int resource_id;
} ProcessInputDescriptor;

typedef struct {
	IOOutputSinkType type;
	int resource_id;
} ProcessOutputDescriptor;

typedef struct {
	ProcessInputDescriptor stdin_desc;
	ProcessOutputDescriptor stdout_desc;
	ProcessOutputDescriptor stderr_desc;
	uint8_t stdin_eof;
	uint8_t stdin_echo; // 1 = hacer echo automatico cuando se lee del teclado, 0 = no hacer echo
} ProcessIOState;

// Process Control Block
typedef struct PCB {
	process_id_t pid;
	char name[MAX_PROCESS_NAME];
	ProcessState state;
	uint8_t priority;				   // 0 (alta) -> 255 (baja)
	uint8_t base_priority;			   // Prioridad original para restaurar tras aging
	uint8_t saved_priority;			   // Prioridad almacenada para restaurar al liberar foreground
	uint8_t foreground_priority_boost; // 1 si priorizamos por foreground
	void *stack_base;				   // Base del stack
	void *stack_pointer;			   // Current stack pointer (RSP)
	void *entry_point;				   // Punto de entrada del proceso
	int argc;						   // Cantidad de argumentos
	char **argv;					   // Array de argumentos
	uint64_t scheduler_counter;		   // Contador para Round Robin
	int in_scheduler;				   // 1 = puede ser elegido por scheduler, 0 = removido
	process_id_t waiting_pid;		   // PID del proceso que esta esperando a este proceso
	process_id_t parent_pid;		   // PID del proceso padre (-1 si no tiene padre)
	uint16_t waiting_ticks;			   // Contador para aging
	uint8_t pending_cleanup;		   // 1 si falta liberar recursos luego del context switch
	uint8_t has_foreground;			   // 1 si el proceso es foreground actual
	ProcessIOState io_state;		   // Estado de IO del proceso
} PCB;

// Informacion de proceso para userland
typedef struct {
	process_id_t pid;
	char name[MAX_PROCESS_NAME];
	uint8_t priority;
	uint64_t stack_base;
	uint64_t rsp;
	char state_name[16];
	uint8_t hasForeground; // 1 = proceso tiene foreground, 0 = background
} ProcessInfo;

/**
 * @brief Inicializa el sistema de procesos
 * @return 0 si la inicializacion es exitosa, -1 si hay error
 */
int init_processes();

/**
 * @brief Verifica si los procesos han sido inicializados
 * @return 1 si estan inicializados, 0 si no
 */
int processes_initialized();

/**
 * @brief Crea un nuevo proceso con los parametros especificados
 * @param name Nombre del proceso
 * @param entry_point Puntero a la funcion de entrada
 * @param argc Numero de argumentos
 * @param argv Array de argumentos
 * @param priority Prioridad del proceso (0-4)
 * @return PID del proceso creado o -1 si hay error
 */
process_id_t create_process(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
							uint8_t priority);

/**
 * @brief Crea un nuevo proceso con configuracion de IO personalizada
 * @param name Nombre del proceso
 * @param entry_point Funcion de entrada
 * @param argc Cantidad de argumentos
 * @param argv Argumentos
 * @param priority Prioridad solicitada
 * @param io_config Configuracion de IO (puede ser NULL para heredar del padre)
 * @return PID del proceso creado o -1 si hay error
 */
process_id_t create_process_with_io(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
									uint8_t priority, const process_io_config_t *io_config);

/**
 * @brief Crea un proceso en foreground con configuracion de IO
 * @param name Nombre del proceso
 * @param entry_point Funcion de entrada
 * @param argc Cantidad de argumentos
 * @param argv Argumentos
 * @param priority Prioridad solicitada
 * @param io_config Configuracion de IO (puede ser NULL)
 * @return PID del proceso creado o -1 si hay error
 */
process_id_t create_process_foreground_with_io(const char *name, void (*entry_point)(int, char **), int argc,
											   char **argv, uint8_t priority, const process_io_config_t *io_config);

/**
 * @brief Obtiene el PID del proceso actualmente ejecutandose
 * @return PID del proceso actual
 */
process_id_t get_current_pid();

/**
 * @brief Registra el PID del proceso en foreground
 * @param pid PID del proceso a marcar como foreground (-1 para limpiar)
 * @return 0 si se actualiza correctamente, -1 en caso contrario
 */
int set_foreground_process(process_id_t pid);

/**
 * @brief Limpia el foreground si coincide con el PID indicado
 * @param pid PID del proceso que libera el foreground
 * @return 0 si se limpia, -1 si no coincide
 */
int clear_foreground_process(process_id_t pid);

/**
 * @brief Obtiene el PID del proceso actualmente en foreground
 * @return PID foreground, -1 si ninguno
 */
process_id_t get_foreground_process(void);

/**
 * @brief Termina un proceso y libera sus recursos
 * @param pid PID del proceso a terminar
 * @return 0 si exitoso, -1 si hay error
 */
int kill_process(process_id_t pid);

/**
 * @brief Bloquea un proceso (cambia estado a BLOCKED)
 * @param pid PID del proceso a bloquear
 * @return 0 si exitoso, -1 si hay error
 */
int block_process(process_id_t pid);

/**
 * @brief Remueve un proceso del scheduler
 * @param process Proceso a remover del scheduler
 * @return 0 si es exitoso, -1 si hay error
 */
int remove_from_scheduler(PCB *process);

/**
 * @brief Agrega un proceso al scheduler
 * @param process Proceso a agregar al scheduler
 * @return 0 si es exitoso, -1 si hay error
 */
int add_to_scheduler(PCB *process);

/**
 * @brief Desbloquea un proceso (cambia estado a READY)
 * @param pid PID del proceso a desbloquear
 * @return 0 si exitoso, -1 si hay error
 */
int unblock_process(process_id_t pid);

/**
 * @brief Cambia la prioridad de un proceso
 * @param pid PID del proceso
 * @param new_priority Nueva prioridad (0-255)
 * @return 0 si exitoso, -1 si hay error
 */
int change_priority(process_id_t pid, uint8_t new_priority);

/**
 * @brief Cede voluntariamente el CPU al scheduler
 */
void yield();

/**
 * @brief Obtiene informacion de todos los procesos activos
 * @param buffer Buffer donde se guardara la informacion
 * @param max_processes Numero maximo de procesos a retornar
 * @return Numero de procesos retornados
 */
int get_processes_info(ProcessInfo *buffer, int max_processes);

/**
 * @brief Libera recursos de procesos terminados
 */
void free_terminated_processes();

/**
 * @brief Scheduler principal - retorna el stack pointer del siguiente proceso
 * @param current_stack_pointer Stack pointer del proceso actual
 * @return Stack pointer del siguiente proceso a ejecutar
 */
void *schedule(void *current_stack_pointer);

/**
 * @brief Obtiene el stack del proceso idle
 * @return Puntero al stack del proceso idle
 */
void *get_idle_stack();

/**
 * @brief Espera a que un proceso hijo termine
 * @param pid PID del proceso hijo a esperar
 * @return 0 si exitoso, -1 si hay error
 */
int waitpid(process_id_t pid);

// Funciones auxiliares para manejo del scheduler
int removeFromScheduler(PCB *process);
int addToScheduler(PCB *process);

/**
 * @brief Obtiene el proceso actual en ejecucion
 * @return PCB del proceso actual, NULL si no hay proceso
 */
PCB *get_current_process(void);

/**
 * @brief Wrapper para manejar la transicion a userland
 * @param argc Cantidad de argumentos
 * @param argv Array de argumentos
 * @param rsp Stack pointer
 * @param proc PCB del proceso
 */
void process_entry_wrapper(int argc, char **argv, void *rsp, PCB *proc);

/**
 * @brief Sale del proceso actual y pasa a userland
 */
int exit_current_process(void);

/**
 * @brief Obtiene el PCB de un proceso por su PID
 * @param pid PID del proceso
 * @return Puntero al PCB o NULL si no existe
 */
PCB *get_process_by_pid(process_id_t pid);

/**
 * @brief Realiza lectura desde un descriptor del proceso
 * @param pid PID del proceso
 * @param fd Descriptor solicitado
 * @param buffer Buffer de destino
 * @param count Cantidad de bytes
 * @return Cantidad leida o -1 si hay error
 */
int process_read(process_id_t pid, int fd, char *buffer, size_t count);

/**
 * @brief Realiza escritura hacia un descriptor del proceso
 * @param pid PID del proceso
 * @param fd Descriptor solicitado
 * @param buffer Buffer origen
 * @param count Cantidad de bytes
 * @param color Color utilizado para stdout/errores
 * @param background Color de fondo
 * @return Cantidad escrita o -1 si hay error
 */
int process_write(process_id_t pid, int fd, const char *buffer, size_t count, size_t color, size_t background);

#endif /* _PROCESS_H_ */
