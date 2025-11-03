#include "include/process.h"
#include "../include/interrupts.h"
#include "../include/stddef.h"
#include "../include/videoDriver.h"
#include "../memory-manager/include/memory_manager.h"

extern void *set_process_stack(int argc, char **argv, void *stack, void *pcb);
extern void idle_process();

static PCB *get_process_by_pid(process_id_t pid);
static int add_to_ready_queue(PCB *process);
static PCB *remove_from_ready_queue();
static void init_ready_queue();

static PCB process_table[MAX_PROCESSES];
static PCB *current_process = NULL;
static PCB *idle_pcb = NULL;
static process_id_t next_pid = 1;			   // Empezar en 1, PID 0 es reservado para idle
static process_id_t foregroundProcessPid = -1; // -1 = ninguno

static process_id_t get_next_valid_pid() {
	process_id_t pid = next_pid++;

	if (pid < 0) {
		next_pid = 1;
		pid = 1;
	}

	return pid;
}
static int initialized_flag = 0;

static PCB *ready_queue[MAX_PROCESSES];
static int ready_queue_head = 0;
static int ready_queue_tail = 0;
static int ready_queue_count = 0;

extern MemoryManagerADT memory_manager;
int init_processes() {
	if (initialized_flag) {
		return 0;
	}

	for (int i = 0; i < MAX_PROCESSES; i++) {
		for (int j = 0; j < sizeof(PCB); j++) {
			((char *) &process_table[i])[j] = 0;
		}
		process_table[i].state = TERMINATED;
		process_table[i].pid = -1;
		process_table[i].priority = 0;
		process_table[i].scheduler_counter = 0;
		process_table[i].stack_base = NULL;
		process_table[i].stack_pointer = NULL;
		process_table[i].entry_point = NULL;
		process_table[i].in_scheduler = 0;
		process_table[i].waiting_pid = -1;
		for (int j = 0; j < MAX_PROCESS_NAME; j++) {
			process_table[i].name[j] = '\0';
		}
	}

	idle_pcb = &process_table[0];
	idle_pcb->pid = 0;
	idle_pcb->state = READY;
	idle_pcb->priority = 0;
	idle_pcb->scheduler_counter = 0;
	idle_pcb->in_scheduler = 1;
	const char *idle_name = "idle";
	int i = 0;
	while (idle_name[i] != '\0' && i < MAX_PROCESS_NAME - 1) {
		idle_pcb->name[i] = idle_name[i];
		i++;
	}
	idle_pcb->name[i] = '\0';
	idle_pcb->entry_point = NULL; // El proceso idle no tiene entry_point userland
	idle_pcb->stack_base = memory_alloc(memory_manager, STACK_SIZE);
	if (idle_pcb->stack_base == NULL) {
		return -1;
	}

	void *stack_top = (void *) ((uint64_t) idle_pcb->stack_base + STACK_SIZE - 8);
	idle_pcb->stack_pointer = set_process_stack(0, NULL, stack_top, (void *) idle_pcb);

	current_process = idle_pcb;
	current_process->state = RUNNING;
	init_ready_queue();

	initialized_flag = 1;
	return 0;
}

int processes_initialized() {
	return initialized_flag;
}

process_id_t create_process(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
							uint8_t priority) {
	if (!initialized_flag || entry_point == NULL) {
		return -1;
	}

	if (priority > MAX_PRIORITY) {
		priority = MAX_PRIORITY;
	}
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
	new_process->pid = get_next_valid_pid();
	new_process->state = READY;
	new_process->priority = priority;
	new_process->scheduler_counter = 0;
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
	new_process->entry_point = entry_point;

	// Copiar argumentos al PCB
	new_process->argc = argc;
	if (argc > 0 && argv != NULL) {
		// Asignar memoria para el array de argumentos
		new_process->argv = (char **) memory_alloc(memory_manager, (argc + 1) * sizeof(char *));
		if (new_process->argv == NULL) {
			new_process->state = TERMINATED;
			return -1;
		}

		// Copia cada argumento
		for (int i = 0; i < argc; i++) {
			if (argv[i] != NULL) {
				int len = 0;
				while (argv[i][len] != '\0')
					len++; // Calcula longitud

				new_process->argv[i] = (char *) memory_alloc(memory_manager, len + 1);
				if (new_process->argv[i] == NULL) {
					// Limpia memoria ya asignada
					for (int j = 0; j < i; j++) {
						memory_free(memory_manager, new_process->argv[j]);
					}
					memory_free(memory_manager, new_process->argv);
					new_process->state = TERMINATED;
					return -1;
				}

				// Copia string
				for (int j = 0; j <= len; j++) {
					new_process->argv[i][j] = argv[i][j];
				}
			}
			else {
				new_process->argv[i] = NULL;
			}
		}
		new_process->argv[argc] = NULL; // Termina array
	}
	else {
		new_process->argv = NULL;
	}

	new_process->stack_base = memory_alloc(memory_manager, STACK_SIZE);
	if (new_process->stack_base == NULL) {
		new_process->state = TERMINATED;
		return -1;
	}
	void *stack_top = (void *) ((uint64_t) new_process->stack_base + STACK_SIZE - 8);
	new_process->stack_pointer =
		set_process_stack(new_process->argc, new_process->argv, stack_top, (void *) new_process);
	if (addToScheduler(new_process) < 0) {
		memory_free(memory_manager, new_process->stack_base);
		new_process->stack_base = NULL;
		new_process->stack_pointer = NULL;
		new_process->state = TERMINATED;
		new_process->pid = -1; // Marcar como slot libre
		return -1;
	}

	return new_process->pid;
}

process_id_t get_current_pid() {
	if (current_process == NULL) {
		return -1;
	}
	return current_process->pid;
}

static PCB *get_process_by_pid(process_id_t pid) {
	for (int i = 0; i < MAX_PROCESSES; i++) {
		if (process_table[i].pid == pid) {
			return &process_table[i];
		}
	}
	return NULL;
}

int kill_process(process_id_t pid) {
	if (pid == 0) {
		return -1; // No se puede matar al proceso idle
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL || process->state == TERMINATED) {
		return 0;
	}

	// Desbloquear proceso que esta esperando a este proceso
	if (process->waiting_pid != -1) {
		PCB *waiting_process = get_process_by_pid(process->waiting_pid);
		if (waiting_process != NULL && waiting_process->state == BLOCKED) {
			waiting_process->state = READY;
			addToScheduler(waiting_process);
		}
		process->waiting_pid = -1;
	}

	process->state = TERMINATED;
	removeFromScheduler(process);

	// Libera memoria de argumentos
	if (process->argv != NULL) {
		for (int i = 0; i < process->argc; i++) {
			if (process->argv[i] != NULL) {
				memory_free(memory_manager, process->argv[i]);
			}
		}
		memory_free(memory_manager, process->argv);
		process->argv = NULL;
	}

	if (process->stack_base != NULL) {
		memory_free(memory_manager, process->stack_base);
		process->stack_base = NULL;
		process->stack_pointer = NULL;
	}

	if (process == current_process) {
		_force_scheduler_interrupt();
	}

	free_terminated_processes();

	return 0;
}

int waitpid(process_id_t pid) {
	if (pid <= 0) {
		return -1;
	}

	PCB *child_process = get_process_by_pid(pid);
	if (child_process == NULL) {
		return -1;
	}

	if (child_process->state == TERMINATED) {
		return 0;
	}

	// Bloquear proceso actual y guardar su PID en el hijo
	PCB *current = get_process_by_pid(get_current_pid());
	if (current == NULL) {
		return -1;
	}

	child_process->waiting_pid = current->pid;

	current->state = BLOCKED;
	removeFromScheduler(current);

	_force_scheduler_interrupt();

	return 0;
}

int block_process(process_id_t pid) {
	if (pid == 0) {
		return -1; // No se puede bloquear al proceso idle
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL || process->state == BLOCKED || process->state == TERMINATED) {
		return 0;
	}

	process->state = BLOCKED;
	removeFromScheduler(process);

	if (process == current_process) {
		_force_scheduler_interrupt();
	}

	return 0;
}

int unblock_process(process_id_t pid) {
	PCB *process = get_process_by_pid(pid);
	if (process == NULL || process->state != BLOCKED) {
		return process == NULL ? -1 : 0;
	}

	process->state = READY;
	return addToScheduler(process);
}
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

int get_processes_info(ProcessInfo *buffer, int max_processes) {
	if (buffer == NULL || max_processes <= 0) {
		return -1;
	}

	int count = 0;
	for (int i = 0; i < MAX_PROCESSES && count < max_processes; i++) {
		if (process_table[i].state != TERMINATED && process_table[i].pid >= 0) {
			buffer[count].pid = process_table[i].pid;
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

			buffer[count].hasForeground = (foregroundProcessPid == process_table[i].pid) ? 1 : 0;

			count++;
		}
	}

	return count;
}

void free_terminated_processes() {
	for (int i = 1; i < MAX_PROCESSES; i++) { // No liberar idle (i=0)
		if (process_table[i].state == TERMINATED) {
			removeFromScheduler(&process_table[i]);

			// Libera memoria de argumentos
			if (process_table[i].argv != NULL) {
				for (int j = 0; j < process_table[i].argc; j++) {
					if (process_table[i].argv[j] != NULL) {
						memory_free(memory_manager, process_table[i].argv[j]);
					}
				}
				memory_free(memory_manager, process_table[i].argv);
			}

			if (process_table[i].stack_base != NULL) {
				memory_free(memory_manager, process_table[i].stack_base);
			}

			process_table[i].stack_base = NULL;
			process_table[i].stack_pointer = NULL;
			process_table[i].entry_point = NULL;
			process_table[i].argc = 0;
			process_table[i].argv = NULL;
			process_table[i].pid = -1;		   // Marcar como slot libre
			process_table[i].in_scheduler = 0; // Limpiar flag del scheduler

			for (int j = 0; j < MAX_PROCESS_NAME; j++) {
				process_table[i].name[j] = '\0';
			}
		}
	}
}

void *schedule(void *current_stack_pointer) {
	if (!initialized_flag) {
		return current_stack_pointer;
	}

	if (current_process != NULL) {
		current_process->stack_pointer = current_stack_pointer;
		if (current_process->state == RUNNING) {
			current_process->state = READY;
			if (add_to_ready_queue(current_process) < 0) {
				// Si la cola está llena, forzar terminación del proceso
				current_process->state = TERMINATED;
			}
		}
	}

	PCB *next_process = remove_from_ready_queue();

	if (next_process == NULL) {
		next_process = idle_pcb;
	}

	current_process = next_process;
	current_process->state = RUNNING;
	current_process->scheduler_counter++;

	return current_process->stack_pointer;
}

void *get_idle_stack() {
	if (idle_pcb == NULL) {
		return NULL;
	}
	return idle_pcb->stack_pointer;
}

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
		return NULL;
	}

	PCB *process = ready_queue[ready_queue_head];
	ready_queue[ready_queue_head] = NULL;
	ready_queue_head = (ready_queue_head + 1) % MAX_PROCESSES;
	ready_queue_count--;

	return process;
}

int removeFromScheduler(PCB *process) {
	if (process == NULL) {
		return -1;
	}

	for (int i = 0; i < ready_queue_count; i++) {
		int index = (ready_queue_head + i) % MAX_PROCESSES;
		if (ready_queue[index] == process) {
			for (int j = i; j < ready_queue_count - 1; j++) {
				int curr = (ready_queue_head + j) % MAX_PROCESSES;
				int next = (ready_queue_head + j + 1) % MAX_PROCESSES;
				ready_queue[curr] = ready_queue[next];
			}
			ready_queue_count--;
			ready_queue_tail = (ready_queue_tail - 1 + MAX_PROCESSES) % MAX_PROCESSES;
			ready_queue[(ready_queue_head + ready_queue_count) % MAX_PROCESSES] = NULL;
			return 1;
		}
	}

	return 1;
}

int addToScheduler(PCB *process) {
	if (process == NULL) {
		return -1;
	}

	if (process->state == READY) {
		return add_to_ready_queue(process);
	}

	return 1;
}

PCB *get_current_process(void) {
	return current_process;
}

void process_entry_wrapper(int argc, char **argv, void *rsp, PCB *proc) {
	if (rsp == NULL || proc == NULL || proc->entry_point == NULL) {
		vd_print("process_entry_wrapper: Invalid arguments\n");
		return;
	}

	// Llamar a la funcion userland con los argumentos del PCB
	((void (*)(int, char **)) proc->entry_point)(proc->argc, proc->argv);

	kill_process(proc->pid);
}
