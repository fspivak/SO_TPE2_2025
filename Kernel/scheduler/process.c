#include "include/process.h"
#include "../include/interrupts.h"
#include "../include/stddef.h"
#include "../memory-manager/include/memory_manager.h"

// Declaraciones externas de funciones en assembly
extern void *set_process_stack(int argc, char **argv, void *stack, void *entry);
extern void idle_process();

// Funciones auxiliares internas
static PCB *get_process_by_pid(process_id_t pid);
static int add_to_ready_queue(PCB *process);
static PCB *remove_from_ready_queue();
static void init_ready_queue();

// Variables globales
static PCB process_table[MAX_PROCESSES];
static PCB *current_process = NULL;
static PCB *idle_pcb = NULL;
static process_id_t next_pid = 1; // Empezar en 1, PID 0 es reservado para idle

// Función para obtener el siguiente PID válido, evitando desbordamiento
static process_id_t get_next_valid_pid() {
	process_id_t pid = next_pid++;

	// Si se desborda, buscar un PID libre en la tabla
	if (pid == 0) {
		// Buscar el primer PID libre empezando desde 1
		for (process_id_t candidate = 1; candidate < MAX_PROCESSES; candidate++) {
			PCB *existing = get_process_by_pid(candidate);
			if (existing == NULL || existing->state == TERMINATED) {
				return candidate;
			}
		}
		// Si no hay PIDs libres, usar un PID aleatorio válido
		return (process_id_t) (1 + (next_pid % (MAX_PROCESSES - 1)));
	}

	return pid;
}
static int initialized_flag = 0;

// Cola simple de procesos READY
static PCB *ready_queue[MAX_PROCESSES];
static int ready_queue_head = 0;
static int ready_queue_tail = 0;
static int ready_queue_count = 0;

// Referencia externa al memory manager
extern MemoryManagerADT memory_manager;

// Inicializa el sistema de procesos
int init_processes() {
	if (initialized_flag) {
		return 0;
	}

	// Inicializar tabla de procesos
	for (int i = 0; i < MAX_PROCESSES; i++) {
		// Limpia completamente la estructura para evitar datos basura
		for (int j = 0; j < sizeof(PCB); j++) {
			((char *) &process_table[i])[j] = 0;
		}

		process_table[i].state = TERMINATED;
		process_table[i].pid = -1; // -1 indica slot libre
		process_table[i].priority = 0;
		process_table[i].quantum_count = 0;
		process_table[i].stack_base = NULL;
		process_table[i].stack_pointer = NULL;
		process_table[i].in_scheduler = 0; // Inicialmente removido del scheduler

		// Limpiar nombre
		for (int j = 0; j < MAX_PROCESS_NAME; j++) {
			process_table[i].name[j] = '\0';
		}
	}

	// Crear proceso idle (PID 0)
	idle_pcb = &process_table[0];
	idle_pcb->pid = 0; // Idle siempre es PID 0
	idle_pcb->state = READY;
	idle_pcb->priority = 0; // Prioridad mas alta
	idle_pcb->quantum_count = 0;
	idle_pcb->in_scheduler = 1; // Idle siempre está en el scheduler

	// Copiar nombre
	const char *idle_name = "idle";
	int i = 0;
	while (idle_name[i] != '\0' && i < MAX_PROCESS_NAME - 1) {
		idle_pcb->name[i] = idle_name[i];
		i++;
	}
	idle_pcb->name[i] = '\0';

	// Asignar stack al proceso idle
	idle_pcb->stack_base = memory_alloc(memory_manager, STACK_SIZE);
	if (idle_pcb->stack_base == NULL) {
		return -1;
	}

	// Configurar stack del proceso idle
	void *stack_top = (void *) ((uint64_t) idle_pcb->stack_base + STACK_SIZE - 8);
	idle_pcb->stack_pointer = set_process_stack(0, NULL, stack_top, (void *) idle_process);

	current_process = idle_pcb;
	current_process->state = RUNNING;

	// Inicializar cola READY
	init_ready_queue();

	initialized_flag = 1;
	return 0;
}

// Verifica si los procesos estan inicializados
int processes_initialized() {
	return initialized_flag;
}

// Crea un nuevo proceso
process_id_t create_process(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
							uint8_t priority) {
	if (!initialized_flag || entry_point == NULL) {
		return -1;
	}

	// Validar prioridad
	if (priority > MAX_PRIORITY) {
		priority = MAX_PRIORITY;
	}

	// Buscar slot libre en la tabla de procesos
	int slot = -1;
	for (int i = 1; i < MAX_PROCESSES; i++) { // Empezar en 1 (idle es 0)
		if (process_table[i].state == TERMINATED) {
			slot = i;
			break;
		}
	}

	if (slot == -1) {
		return -1; // No hay slots disponibles
	}

	PCB *new_process = &process_table[slot];

	// Configurar PCB
	new_process->pid = get_next_valid_pid();
	new_process->state = READY;
	new_process->priority = priority;
	new_process->quantum_count = 0;

	// Copiar nombre del proceso
	if (name != NULL) {
		int i = 0;
		while (name[i] != '\0' && i < MAX_PROCESS_NAME - 1) {
			new_process->name[i] = name[i];
			i++;
		}
		new_process->name[i] = '\0';
	}
	else {
		new_process->name[0] = '?';
		new_process->name[1] = '\0';
	}

	// Asignar stack
	new_process->stack_base = memory_alloc(memory_manager, STACK_SIZE);
	if (new_process->stack_base == NULL) {
		new_process->state = TERMINATED;
		return -1;
	}

	// Configurar stack inicial
	void *stack_top = (void *) ((uint64_t) new_process->stack_base + STACK_SIZE - 8);
	new_process->stack_pointer = set_process_stack(argc, argv, stack_top, (void *) entry_point);

	// Agregar al scheduler
	if (addToScheduler(new_process) < 0) {
		// Si falla al agregar al scheduler, limpiar completamente
		memory_free(memory_manager, new_process->stack_base);
		new_process->stack_base = NULL;
		new_process->stack_pointer = NULL;
		new_process->state = TERMINATED;
		new_process->pid = -1; // Marcar como slot libre
		return -1;
	}

	return new_process->pid;
}

// Obtiene el PID del proceso actual
process_id_t get_current_pid() {
	if (current_process == NULL) {
		return -1;
	}
	return current_process->pid;
}

// Busca un proceso por PID
static PCB *get_process_by_pid(process_id_t pid) {
	for (int i = 0; i < MAX_PROCESSES; i++) {
		if (process_table[i].pid == pid) { // Incluir procesos TERMINATED
			return &process_table[i];
		}
	}
	return NULL;
}

// Termina un proceso
int kill_process(process_id_t pid) {
	if (pid == 0) {
		return -1; // No se puede matar al proceso idle
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return 0; // Proceso no encontrado = ya está "muerto"
	}

	if (process->state == TERMINATED) {
		return 0;
	}

	// FILOSOFÍA: Matar un proceso SIEMPRE es exitoso, independientemente del estado
	process->state = TERMINATED;

	// Remover del scheduler DESPUÉS de cambiar estado
	if (removeFromScheduler(process) < 0) {
		// Si falla, continuar de todas formas
	}

	// Liberar stack DESPUÉS de cambiar estado
	if (process->stack_base != NULL) {
		memory_free(memory_manager, process->stack_base);
		process->stack_base = NULL;
		process->stack_pointer = NULL;
	}

	// Si el proceso que estamos matando es el actual, forzar context switch
	if (process == current_process) {
		_force_scheduler_interrupt();
	}

	// Liberar procesos terminados después de matar uno
	free_terminated_processes();

	return 0;
}

// Bloquea un proceso
int block_process(process_id_t pid) {
	if (pid == 0) {
		return -1; // No se puede bloquear al proceso idle
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return 0; // Proceso no encontrado = ya está "bloqueado"
	}

	// Si ya está bloqueado o terminado, considerar como éxito
	if (process->state == BLOCKED || process->state == TERMINATED) {
		return 0;
	}

	// Bloquear un proceso SIEMPRE es exitoso

	process->state = BLOCKED;

	// Remover del scheduler DESPUÉS de cambiar estado
	if (removeFromScheduler(process) < 0) {
		// Si falla, continuar de todas formas - el proceso ya está BLOCKED
	}

	// Si el proceso bloqueado es el que está ejecutándose actualmente,
	// forzamos un context switch
	if (process == current_process) {
		_force_scheduler_interrupt();
	}

	return 0;
}

// Desbloquea un proceso
int unblock_process(process_id_t pid) {
	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return -1; // Proceso no encontrado
	}

	// Si ya no está bloqueado, considerar como éxito
	if (process->state != BLOCKED) {
		return 0;
	}

	process->state = READY;

	// Agregar al scheduler
	return addToScheduler(process);
}

// Cambia la prioridad de un proceso
int change_priority(process_id_t pid, uint8_t new_priority) {
	if (new_priority > MAX_PRIORITY) {
		return -1;
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return -1;
	}

	process->priority = new_priority;
	return 0;
}

// Obtiene informacion de todos los procesos
int get_processes_info(ProcessInfo *buffer, int max_processes) {
	if (buffer == NULL || max_processes <= 0) {
		return -1;
	}

	int count = 0;
	for (int i = 0; i < MAX_PROCESSES && count < max_processes; i++) {
		// Solo incluir procesos validos (no terminados, PID valido, y nombre valido)
		if (process_table[i].state != TERMINATED && process_table[i].pid >= 0 && process_table[i].pid < 10000 &&
			process_table[i].name[0] != '\0' && process_table[i].name[0] >= 32 &&
			process_table[i].name[0] <= 126) { // Solo caracteres imprimibles ASCII
			buffer[count].pid = process_table[i].pid;

			// Copiar nombre
			int j = 0;
			while (j < MAX_PROCESS_NAME - 1) {
				buffer[count].name[j] = process_table[i].name[j];
				if (process_table[i].name[j] == '\0')
					break;
				j++;
			}
			buffer[count].name[MAX_PROCESS_NAME - 1] = '\0';

			buffer[count].priority = process_table[i].priority;
			buffer[count].stack_base = (uint64_t) process_table[i].stack_base;
			buffer[count].rsp = (uint64_t) process_table[i].stack_pointer;

			// Estado como string
			const char *state_str;
			switch (process_table[i].state) {
				case READY:
					state_str = "READY";
					break;
				case RUNNING:
					state_str = "RUNNING";
					break;
				case BLOCKED:
					state_str = "BLOCKED";
					break;
				default:
					state_str = "UNKNOWN";
					break;
			}

			j = 0;
			while (state_str[j] != '\0' && j < 15) {
				buffer[count].state_name[j] = state_str[j];
				j++;
			}
			buffer[count].state_name[j] = '\0';

			count++;
		}
	}

	return count;
}

// Libera procesos terminados
void free_terminated_processes() {
	for (int i = 1; i < MAX_PROCESSES; i++) { // No liberar idle (i=0)
		if (process_table[i].state == TERMINATED) {
			// Remover del scheduler ANTES de limpiar la memoria
			removeFromScheduler(&process_table[i]);

			// Liberar stack si existe
			if (process_table[i].stack_base != NULL) {
				memory_free(memory_manager, process_table[i].stack_base);
			}

			process_table[i].stack_base = NULL;
			process_table[i].stack_pointer = NULL;
			process_table[i].pid = -1;		   // Marcar como slot libre
			process_table[i].in_scheduler = 0; // Limpiar flag del scheduler

			// Limpiar nombre para evitar datos basura
			for (int j = 0; j < MAX_PROCESS_NAME; j++) {
				process_table[i].name[j] = '\0';
			}
		}
	}
}

// Scheduler Round Robin con prioridades
void *schedule(void *current_stack_pointer) {
	if (!initialized_flag) {
		return current_stack_pointer;
	}

	// Guardar stack pointer del proceso actual
	if (current_process != NULL) {
		current_process->stack_pointer = current_stack_pointer;
		if (current_process->state == RUNNING) {
			current_process->state = READY;
			// Agregar a la cola READY - manejar error si está llena
			if (add_to_ready_queue(current_process) < 0) {
				// Si la cola está llena, forzar terminación del proceso
				current_process->state = TERMINATED;
			}
		}
	}

	// Seleccionar siguiente proceso desde la cola READY
	PCB *next_process = remove_from_ready_queue();

	// Si no hay procesos READY, usar idle
	if (next_process == NULL) {
		next_process = idle_pcb;
	}

	// Cambiar al nuevo proceso
	current_process = next_process;
	current_process->state = RUNNING;
	current_process->quantum_count++;

	return current_process->stack_pointer;
}

// Obtiene el stack del proceso idle
void *get_idle_stack() {
	if (idle_pcb == NULL) {
		return NULL;
	}
	return idle_pcb->stack_pointer;
}

// Funciones de manejo de cola READY
static void init_ready_queue() {
	ready_queue_head = 0;
	ready_queue_tail = 0;
	ready_queue_count = 0;
	for (int i = 0; i < MAX_PROCESSES; i++) {
		ready_queue[i] = NULL;
	}
}

static int add_to_ready_queue(PCB *process) {
	if (process == NULL) {
		return -1;
	}

	// Si la cola está llena, error
	if (ready_queue_count >= MAX_PROCESSES) {
		return -1;
	}

	ready_queue[ready_queue_tail] = process;
	ready_queue_tail = (ready_queue_tail + 1) % MAX_PROCESSES;
	ready_queue_count++;
	return 1;
}

static PCB *remove_from_ready_queue() {
	if (ready_queue_count == 0) {
		return NULL; // No hay procesos, retornar NULL silenciosamente
	}

	PCB *process = ready_queue[ready_queue_head];
	ready_queue[ready_queue_head] = NULL;
	ready_queue_head = (ready_queue_head + 1) % MAX_PROCESSES;
	ready_queue_count--;

	return process;
}

// Remueve un proceso del scheduler
int removeFromScheduler(PCB *process) {
	if (process == NULL) {
		return -1;
	}

	// Buscar y remover de la cola, sin importar el estado actual
	for (int i = 0; i < ready_queue_count; i++) {
		int index = (ready_queue_head + i) % MAX_PROCESSES;
		if (ready_queue[index] == process) {
			// Mover todos los elementos hacia atrás
			for (int j = i; j < ready_queue_count - 1; j++) {
				int curr = (ready_queue_head + j) % MAX_PROCESSES;
				int next = (ready_queue_head + j + 1) % MAX_PROCESSES;
				ready_queue[curr] = ready_queue[next];
			}
			ready_queue_count--;
			ready_queue_tail = (ready_queue_tail - 1 + MAX_PROCESSES) % MAX_PROCESSES;
			// Limpiar el último puntero
			ready_queue[(ready_queue_head + ready_queue_count) % MAX_PROCESSES] = NULL;
			return 1; // Proceso removido exitosamente
		}
	}

	return 1;
}

// Agrega un proceso al scheduler
int addToScheduler(PCB *process) {
	if (process == NULL) {
		return -1;
	}

	// Solo agregar si está READY
	if (process->state == READY) {
		return add_to_ready_queue(process);
	}

	return 1;
}
