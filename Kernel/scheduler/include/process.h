#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <stdint.h>

#define MAX_PROCESSES 64
#define STACK_SIZE 4096 // 4KB por process stack
#define MAX_PROCESS_NAME 32
#define MIN_PRIORITY 0
#define MAX_PRIORITY 4

typedef uint16_t process_id_t;

// Estados de proceso
typedef enum {
	READY,	   // Listo para ejecutar
	RUNNING,   // Ejecutando actualmente
	BLOCKED,   // Bloqueado (esperando algo)
	TERMINATED // Terminado (liberar recursos)
} ProcessState;

// Process Control Block
typedef struct PCB {
	process_id_t pid;
	char name[MAX_PROCESS_NAME];
	ProcessState state;
	uint8_t priority;		// 0 (alta) -> 4 (baja)
	void *stack_base;		// Base del stack
	void *stack_pointer;	// Current stack pointer (RSP)
	uint64_t quantum_count; // Para Round Robin con prioridades
} PCB;

// Informacion de proceso para userland
typedef struct {
	process_id_t pid;
	char name[MAX_PROCESS_NAME];
	uint8_t priority;
	uint64_t stack_base;
	uint64_t rsp;
	char state_name[16];
} ProcessInfo;

/* Inicializa el sistema de procesos */
int init_processes();

/* Verifica si los procesos han sido inicializados */
int processes_initialized();

/* Crea un nuevo proceso */
process_id_t create_process(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
							uint8_t priority);

/* Obtiene el PID del proceso actual */
process_id_t get_current_pid();

/* Termina un proceso */
int kill_process(process_id_t pid);

/* Bloquea un proceso */
int block_process(process_id_t pid);

/* Desbloquea un proceso */
int unblock_process(process_id_t pid);

/* Cambia la prioridad de un proceso */
int change_priority(process_id_t pid, uint8_t new_priority);

/* Cede voluntariamente el CPU */
void yield();

/* Obtiene informacion de todos los procesos */
int get_processes_info(ProcessInfo *buffer, int max_processes);

/* Libera procesos terminados */
void free_terminated_processes();

/* Scheduler principal - retorna el stack pointer del siguiente proceso */
void *schedule(void *current_stack_pointer);

/* Obtiene el stack del proceso idle */
void *get_idle_stack();

#endif /* _PROCESS_H_ */
