#include "include/process.h"
#include "../include/interrupts.h"
#include "../include/stddef.h"
#include "../include/videoDriver.h"
#include "../memory-manager/include/memory_manager.h"

extern void *set_process_stack(int argc, char **argv, void *stack, void *pcb);
extern void idle_process();

#define USER_STACK_GUARD_SIZE 256
#define CONTEXT_FRAME_QWORDS 21
#define CONTEXT_FRAME_SIZE (CONTEXT_FRAME_QWORDS * sizeof(uint64_t))

static PCB *get_process_by_pid(process_id_t pid);
static int terminate_process(PCB *process, int kill_descendants);
static int add_to_ready_queue(PCB *process);
static PCB *remove_from_ready_queue();
static void init_ready_queue();
static void apply_aging_to_ready_processes(void);
static void reset_process_name(PCB *process);
static void release_process_arguments(PCB *process);
static void release_process_resources(PCB *process);
static int duplicate_arguments(PCB *process, int argc, char **argv);
static uint8_t clamp_priority(uint8_t priority);
static uint8_t normalize_priority(uint8_t priority);
static uint8_t get_time_slice_budget(const PCB *process);
static void release_foreground_owner(void);

static PCB process_table[MAX_PROCESSES];
static PCB *current_process = NULL;
static PCB *idle_pcb = NULL;
static process_id_t next_pid = 1;			   // Empezar en 1, PID 0 es reservado para idle
static process_id_t foregroundProcessPid = -1; // -1 = ninguno
#define AGING_THRESHOLD 5
#define AGING_STEP 32
#define PRIORITY_BUCKETS 11

static const uint8_t priority_time_slices[PRIORITY_BUCKETS] = {4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1};

static PCB *pending_cleanup_process = NULL;

static process_id_t get_next_valid_pid() {
	process_id_t pid = next_pid++;

	if (pid < 0) {
		next_pid = 1;
		pid = 1;
	}

	return pid;
}

static void release_foreground_owner(void) {
	if (foregroundProcessPid == -1) {
		return;
	}

	PCB *current_foreground = get_process_by_pid(foregroundProcessPid);
	if (current_foreground != NULL) {
		current_foreground->has_foreground = 0;
		if (current_foreground->foreground_priority_boost) {
			current_foreground->priority = current_foreground->saved_priority;
			current_foreground->foreground_priority_boost = 0;
		}
	}

	foregroundProcessPid = -1;
}
static int initialized_flag = 0;

static PCB *ready_queue[MAX_PROCESSES];
static int ready_queue_head = 0;
static int ready_queue_tail = 0;
static int ready_queue_count = 0;
static int cleanup_counter = 0; // Contador para limpieza periódica

extern MemoryManagerADT memory_manager;

static void reset_process_name(PCB *process) {
	if (process == NULL) {
		return;
	}

	for (int i = 0; i < MAX_PROCESS_NAME; i++) {
		process->name[i] = '\0';
	}
}

static void release_process_arguments(PCB *process) {
	if (process == NULL || process->argv == NULL) {
		return;
	}

	for (int i = 0; i < process->argc; i++) {
		if (process->argv[i] != NULL) {
			memory_free(memory_manager, process->argv[i]);
			process->argv[i] = NULL;
		}
	}

	memory_free(memory_manager, process->argv);
	process->argv = NULL;
	process->argc = 0;
}

static void release_process_resources(PCB *process) {
	if (process == NULL) {
		return;
	}

	removeFromScheduler(process);

	release_process_arguments(process);

	if (process->stack_base != NULL) {
		memory_free(memory_manager, process->stack_base);
		process->stack_base = NULL;
		process->stack_pointer = NULL;
	}

	process->entry_point = NULL;
	process->scheduler_counter = 0;
	process->in_scheduler = 0;
	process->waiting_pid = -1;
	process->parent_pid = -1;
	process->pid = -1;
	process->priority = 0;
	process->base_priority = 0;
	process->saved_priority = 0;
	process->foreground_priority_boost = 0;
	process->waiting_ticks = 0;
	process->pending_cleanup = 0;
	process->has_foreground = 0;

	if (pending_cleanup_process == process) {
		pending_cleanup_process = NULL;
	}

	reset_process_name(process);
}

static int duplicate_arguments(PCB *process, int argc, char **argv) {
	if (process == NULL) {
		return -1;
	}

	process->argc = 0;
	process->argv = NULL;

	if (argc <= 0 || argv == NULL) {
		return 0;
	}

	process->argv = (char **) memory_alloc(memory_manager, (argc + 1) * sizeof(char *));
	if (process->argv == NULL) {
		return -1;
	}

	for (int i = 0; i < argc; i++) {
		process->argv[i] = NULL;
	}

	for (int i = 0; i < argc; i++) {
		if (argv[i] == NULL) {
			continue;
		}

		int len = 0;
		while (argv[i][len] != '\0') {
			len++;
		}

		process->argv[i] = (char *) memory_alloc(memory_manager, len + 1);
		if (process->argv[i] == NULL) {
			release_process_arguments(process);
			return -1;
		}

		for (int j = 0; j <= len; j++) {
			process->argv[i][j] = argv[i][j];
		}
	}

	process->argv[argc] = NULL;
	process->argc = argc;

	return 0;
}

static uint8_t clamp_priority(uint8_t priority) {
	if (priority > MAX_PRIORITY) {
		return MAX_PRIORITY;
	}
	return priority;
}

static uint8_t normalize_priority(uint8_t priority) {
	priority = clamp_priority(priority);
	return (uint8_t) ((priority * (PRIORITY_BUCKETS - 1) + (MAX_PRIORITY / 2)) / MAX_PRIORITY);
}

static uint8_t get_time_slice_budget(const PCB *process) {
	if (process == NULL) {
		return 1;
	}
	uint8_t normalized = normalize_priority(process->priority);
	uint8_t budget = priority_time_slices[normalized];
	if (budget == 0) {
		budget = 1;
	}
	return budget;
}
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
		process_table[i].base_priority = 0;
		process_table[i].saved_priority = 0;
		process_table[i].foreground_priority_boost = 0;
		process_table[i].scheduler_counter = 0;
		process_table[i].stack_base = NULL;
		process_table[i].stack_pointer = NULL;
		process_table[i].entry_point = NULL;
		process_table[i].in_scheduler = 0;
		process_table[i].waiting_pid = -1;
		process_table[i].parent_pid = -1;
		process_table[i].waiting_ticks = 0;
		process_table[i].pending_cleanup = 0;
		process_table[i].has_foreground = 0;
		for (int j = 0; j < MAX_PROCESS_NAME; j++) {
			process_table[i].name[j] = '\0';
		}
	}

	idle_pcb = &process_table[0];
	idle_pcb->pid = 0;
	idle_pcb->state = READY;
	idle_pcb->priority = 0;
	idle_pcb->base_priority = 0;
	idle_pcb->saved_priority = 0;
	idle_pcb->foreground_priority_boost = 0;
	idle_pcb->scheduler_counter = 0;
	idle_pcb->in_scheduler = 1;
	idle_pcb->has_foreground = 0;
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

	uint64_t idle_stack_limit = (uint64_t) idle_pcb->stack_base + STACK_SIZE;
	uint64_t idle_user_rsp = (idle_stack_limit - USER_STACK_GUARD_SIZE) & ~((uint64_t) 0xF);
	void *idle_frame_pointer = (void *) idle_user_rsp;
	idle_pcb->stack_pointer = set_process_stack(0, NULL, idle_frame_pointer, (void *) idle_pcb);
	idle_pcb->parent_pid = -1; // Idle no tiene padre

	current_process = idle_pcb;
	current_process->state = RUNNING;
	init_ready_queue();
	pending_cleanup_process = NULL;

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
	// Limpiar procesos terminados antes de buscar un slot para mejorar disponibilidad de memoria
	free_terminated_processes();

	int slot = -1;
	for (int i = 1; i < MAX_PROCESSES; i++) { // Empezar en 1 (idle es 0)
		if (process_table[i].state == TERMINATED) {
			slot = i;
			break;
		}
	}

	if (slot == -1) {
		return -1;
	}

	PCB *new_process = &process_table[slot];
	new_process->pid = get_next_valid_pid();
	new_process->state = READY;
	new_process->priority = priority;
	new_process->base_priority = priority;
	new_process->saved_priority = priority;
	new_process->foreground_priority_boost = 0;
	new_process->scheduler_counter = 0;
	new_process->waiting_ticks = 0;
	new_process->pending_cleanup = 0;
	new_process->parent_pid = get_current_pid();
	new_process->has_foreground = 0;
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

	if (duplicate_arguments(new_process, argc, argv) < 0) {
		new_process->state = TERMINATED;
		return -1;
	}

	new_process->stack_base = memory_alloc(memory_manager, STACK_SIZE);
	if (new_process->stack_base == NULL) {
		free_terminated_processes();
		new_process->stack_base = memory_alloc(memory_manager, STACK_SIZE);
		if (new_process->stack_base == NULL) {
			release_process_arguments(new_process);
			new_process->state = TERMINATED;
			return -1;
		}
	}
	uint64_t stack_limit = (uint64_t) new_process->stack_base + STACK_SIZE;
	uint64_t user_rsp = (stack_limit - USER_STACK_GUARD_SIZE) & ~((uint64_t) 0xF);
	void *stack_top = (void *) user_rsp;
	new_process->stack_pointer =
		set_process_stack(new_process->argc, new_process->argv, stack_top, (void *) new_process);
	if (addToScheduler(new_process) < 0) {
		release_process_resources(new_process);
		new_process->state = TERMINATED;
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

int set_foreground_process(process_id_t pid) {
	if (pid == -1) {
		release_foreground_owner();
		return 0;
	}

	if (pid <= 0) {
		return -1;
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return -1;
	}

	if (process->state == TERMINATED) {
		release_foreground_owner();
		return 0;
	}

	if (foregroundProcessPid == pid) {
		process->has_foreground = 1;
		return 0;
	}

	process_id_t previous_pid = foregroundProcessPid;
	PCB *previous_process = NULL;
	if (previous_pid > 0) {
		previous_process = get_process_by_pid(previous_pid);
	}

	foregroundProcessPid = pid;
	process->has_foreground = 1;
	if (!process->foreground_priority_boost) {
		process->saved_priority = process->priority;
		process->priority = MIN_PRIORITY;
		process->foreground_priority_boost = 1;
	}

	if (previous_process != NULL) {
		previous_process->has_foreground = 0;
		if (previous_process->foreground_priority_boost) {
			previous_process->priority = previous_process->saved_priority;
			previous_process->foreground_priority_boost = 0;
		}
	}

	return 0;
}

int clear_foreground_process(process_id_t pid) {
	if (pid == -1) {
		release_foreground_owner();
		return 0;
	}

	if (foregroundProcessPid == -1) {
		return 0;
	}

	if (foregroundProcessPid != pid) {
		return -1;
	}

	release_foreground_owner();
	return 0;
}

process_id_t get_foreground_process(void) {
	return foregroundProcessPid;
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
		return -1;
	}

	PCB *process = get_process_by_pid(pid);
	return terminate_process(process, 1);
}

int exit_current_process(void) {
	PCB *process = current_process;
	if (process == NULL) {
		return -1;
	}
	return terminate_process(process, 0);
}

static int terminate_process(PCB *process, int kill_descendants) {
	if (process == NULL || process->state == TERMINATED) {
		return (process == NULL) ? -1 : 0;
	}

	process_id_t pid = process->pid;

	for (int i = 0; i < MAX_PROCESSES; i++) {
		PCB *p = &process_table[i];
		if (p->state == BLOCKED && p->waiting_pid == pid) {
			p->state = READY;
			p->waiting_pid = -1;
			addToScheduler(p);
		}
	}
	process->waiting_pid = -1;

	if (kill_descendants) {
		for (int i = 1; i < MAX_PROCESSES; i++) {
			if (process_table[i].parent_pid == pid && process_table[i].state != TERMINATED) {
				terminate_process(&process_table[i], 1);
			}
		}
	}
	else {
		for (int i = 1; i < MAX_PROCESSES; i++) {
			if (process_table[i].parent_pid == pid && process_table[i].state != TERMINATED) {
				process_table[i].parent_pid = process->parent_pid;
			}
		}
	}

	if (foregroundProcessPid == pid) {
		release_foreground_owner();
	}

	process->state = TERMINATED;
	process->in_scheduler = 0;
	process->pending_cleanup = (process == current_process) ? 1 : 0;
	process->has_foreground = 0;
	if (process == current_process) {
		pending_cleanup_process = process;
	}

	removeFromScheduler(process);

	if (process == current_process) {
		_force_scheduler_interrupt();
	}

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

	PCB *current = get_process_by_pid(get_current_pid());
	if (current == NULL) {
		return -1;
	}

	//////////////////////////////////////
	/*child_process->waiting_pid = current->pid;*/

	current->waiting_pid = pid;

	// Verificar nuevamente si el hijo terminó después de establecer waiting_pid
	// Esto evita race conditions donde el hijo termina entre la verificación y el bloqueo
	child_process = get_process_by_pid(pid);
	if (child_process != NULL && child_process->state == TERMINATED) {
		current->waiting_pid = -1;
		return 0;
	}
	//////////////////////////////////////

	current->state = BLOCKED;
	removeFromScheduler(current);

	_force_scheduler_interrupt();

	return 0;
}

int block_process(process_id_t pid) {
	if (pid == 0) {
		return -1;
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL || process->state == TERMINATED) {
		return -1;
	}

	// Return 2 if already blocked, 1 if we blocked it, -1 on error
	if (process->state == BLOCKED) {
		return 2;
	}

	process->state = BLOCKED;
	process->waiting_ticks = 0;
	removeFromScheduler(process);

	if (process == current_process) {
		_force_scheduler_interrupt();
	}

	return 1;
}

int unblock_process(process_id_t pid) {
	PCB *process = get_process_by_pid(pid);
	if (process == NULL || process->state != BLOCKED) {
		return process == NULL ? -1 : 0;
	}

	process->state = READY;
	process->waiting_ticks = 0;
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

	if (process->state == TERMINATED) {
		return -1;
	}
	process->base_priority = new_priority;
	if (process->foreground_priority_boost) {
		process->saved_priority = new_priority;
	}
	else {
		process->priority = new_priority;
	}
	process->waiting_ticks = 0;

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

			buffer[count].priority = process_table[i].base_priority;
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

			buffer[count].hasForeground =
				(foregroundProcessPid == process_table[i].pid) ? 1 : process_table[i].has_foreground;

			count++;
		}
	}

	return count;
}

void free_terminated_processes() {
	for (int i = 1; i < MAX_PROCESSES; i++) { // No liberar idle (i=0)
		if (process_table[i].state == TERMINATED) {
			if (&process_table[i] == current_process) {
				continue; // Evitar liberar el stack del proceso actual
			}
			release_process_resources(&process_table[i]);
		}
	}
}
void *schedule(void *current_stack_pointer) {
	if (!initialized_flag) {
		return current_stack_pointer;
	}

	// Limpiar procesos terminados periódicamente
	cleanup_counter++;
	if (cleanup_counter >= CLEANUP_INTERVAL) {
		free_terminated_processes();
		cleanup_counter = 0;
	}

	if (pending_cleanup_process != NULL && pending_cleanup_process != current_process) {
		release_process_resources(pending_cleanup_process);
		pending_cleanup_process = NULL;
	}

	if (current_process != NULL) {
		current_process->stack_pointer = current_stack_pointer;
		if (current_process->state == RUNNING) {
			uint8_t budget = get_time_slice_budget(current_process);
			current_process->scheduler_counter++;

			if (current_process->scheduler_counter < budget) {
				current_process->state = RUNNING;
				return current_process->stack_pointer;
			}

			current_process->scheduler_counter = 0;
			current_process->state = READY;
			if (add_to_ready_queue(current_process) < 0) {
				current_process->state = TERMINATED;
			}
		}
	}

	apply_aging_to_ready_processes();

	PCB *next_process = remove_from_ready_queue();

	while (next_process != NULL && next_process->state == TERMINATED) {
		next_process = remove_from_ready_queue();
	}

	if (next_process == NULL) {
		next_process = idle_pcb;
	}

	if (next_process->state == TERMINATED || next_process->stack_base == NULL || next_process->stack_pointer == NULL) {
		next_process = idle_pcb;
	}

	current_process = next_process;
	current_process->state = RUNNING;
	current_process->scheduler_counter++;
	current_process->waiting_ticks = 0;
	current_process->priority = clamp_priority(current_process->base_priority);

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

	if (process->state == TERMINATED) {
		return -1;
	}

	if (ready_queue_count >= MAX_PROCESSES) {
		return -1;
	}

	ready_queue[ready_queue_tail] = process;
	ready_queue_tail = (ready_queue_tail + 1) % MAX_PROCESSES;
	ready_queue_count++;
	process->in_scheduler = 1;
	process->waiting_ticks = 0;
	return 1;
}

static PCB *remove_from_ready_queue() {
	if (ready_queue_count == 0) {
		return NULL;
	}

	int lowest_priority = (int) MAX_PRIORITY + 1;
	for (int i = 0; i < ready_queue_count; i++) {
		int index = (ready_queue_head + i) % MAX_PROCESSES;
		PCB *process = ready_queue[index];
		if (process != NULL && process->state != TERMINATED) {
			if ((int) process->priority < lowest_priority) {
				lowest_priority = (int) process->priority;
			}
		}
	}

	for (int i = 0; i < ready_queue_count; i++) {
		int index = (ready_queue_head + i) % MAX_PROCESSES;
		PCB *process = ready_queue[index];
		if (process != NULL && process->state != TERMINATED) {
			if ((int) process->priority == lowest_priority) {
				int selected_index = i;
				for (int j = selected_index; j < ready_queue_count - 1; j++) {
					int curr = (ready_queue_head + j) % MAX_PROCESSES;
					int next = (ready_queue_head + j + 1) % MAX_PROCESSES;
					ready_queue[curr] = ready_queue[next];
				}

				ready_queue_count--;
				ready_queue_tail = (ready_queue_tail - 1 + MAX_PROCESSES) % MAX_PROCESSES;
				ready_queue[(ready_queue_head + ready_queue_count) % MAX_PROCESSES] = NULL;

				return process;
			}
		}
	}

	PCB *process = ready_queue[ready_queue_head];
	ready_queue[ready_queue_head] = NULL;
	ready_queue_head = (ready_queue_head + 1) % MAX_PROCESSES;
	ready_queue_count--;
	return process;
}

static void apply_aging_to_ready_processes(void) {
	for (int i = 0; i < ready_queue_count; i++) {
		int index = (ready_queue_head + i) % MAX_PROCESSES;
		PCB *process = ready_queue[index];

		if (process == NULL || process->state != READY) {
			continue;
		}

		if (process->priority == MIN_PRIORITY) {
			process->waiting_ticks = 0;
			continue;
		}

		process->waiting_ticks++;

		if (process->waiting_ticks >= AGING_THRESHOLD) {
			if (process->priority > MIN_PRIORITY) {
				if (process->priority > MIN_PRIORITY + AGING_STEP) {
					process->priority -= AGING_STEP;
				}
				else {
					process->priority = MIN_PRIORITY;
				}
			}
			process->waiting_ticks = 0;
		}
	}
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
			process->in_scheduler = 0;
			return 1;
		}
	}

	return 1;
}

int addToScheduler(PCB *process) {
	if (process == NULL) {
		return -1;
	}

	if (process->state == TERMINATED) {
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

void *get_current_process_stack_pointer(void) {
	if (current_process == NULL) {
		return NULL;
	}
	return current_process->stack_pointer;
}

void process_entry_wrapper(int argc, char **argv, void *rsp, PCB *proc) {
	if (rsp == NULL || proc == NULL || proc->entry_point == NULL) {
		vd_print("process_entry_wrapper: Invalid arguments\n");
		return;
	}

	((void (*)(int, char **)) proc->entry_point)(proc->argc, proc->argv);

	exit_current_process();
	__builtin_unreachable();
}
