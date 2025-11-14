#include "include/process.h"
#include "../include/interrupts.h"
#include "../include/pipe.h"
#include "../include/stddef.h"
#include "../include/stdinout.h"
#include "../include/stringKernel.h"
#include "../include/syscallDispatcher.h"
#include "../include/videoDriver.h"
#include "../memory-manager/include/memory_manager.h"
#include "include/semaphore.h"

// Flag para habilitar logs de debug de pipes
#define PIPE_DEBUG 0 // Deshabilitado - cambiar a 1 para habilitar logs

extern void *set_process_stack(int argc, char **argv, void *stack, void *pcb);
extern void idle_process();

#define USER_STACK_GUARD_SIZE 256
#define CONTEXT_FRAME_QWORDS 21
#define CONTEXT_FRAME_SIZE (CONTEXT_FRAME_QWORDS * sizeof(uint64_t))

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
static void init_process_io_defaults(ProcessIOState *io_state);
static int retain_process_io_resources(PCB *process);
static void release_process_io_resources(PCB *process);
static int override_process_io(PCB *process, const process_io_config_t *config);
static int set_process_stdin_descriptor(PCB *process, uint32_t type, int32_t resource);
static int set_process_stdout_descriptor(PCB *process, uint32_t type, int32_t resource);
static int set_process_stderr_descriptor(PCB *process, uint32_t type, int32_t resource);
static process_id_t create_process_internal(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
											uint8_t priority, const process_io_config_t *io_config,
											int assign_foreground);

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
static int cleanup_counter = 0; // Contador para limpieza periodica

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
// static void release_process_resources(PCB *process) {
// 	if (process == NULL) {
// 		return;
// 	}

// 	// Sacarlo del scheduler si estuviera
// 	removeFromScheduler(process);

// 	// Liberar argv/argc
// 	release_process_arguments(process);

// 	// Liberar stack si existe
// 	if (process->stack_base != NULL && memory_manager != NULL) {
// 		uint64_t stack_base_addr = (uint64_t) process->stack_base;

// 		// Ubicación donde guardamos el puntero original de memory_alloc:
// 		// [ stack_base - sizeof(void*) ]  == raw
// 		uint64_t ptr_location = stack_base_addr - sizeof(void *);

// 		// Chequeos defensivos: que esté en rango razonable y alineado
// 		if (ptr_location >= 0x600000 &&
// 			ptr_location <  0x800000 &&
// 			ptr_location <  stack_base_addr &&
// 			(ptr_location % sizeof(void *)) == 0) {

// 			void *original_ptr = *((void **) ptr_location);

// 			// Solo liberamos si parece un puntero de heap razonable
// 			if (original_ptr != NULL &&
// 				(uint64_t) original_ptr >= 0x600000 &&
// 				(uint64_t) original_ptr <  0x800000 &&
// 				(uint64_t) original_ptr <= stack_base_addr) {

// 				memory_free(memory_manager, original_ptr);
// 			}
// 		}

// 		process->stack_base = NULL;
// 		process->stack_pointer = NULL;
// 	}

// 	// Resetear resto de campos
// 	process->entry_point = NULL;
// 	process->scheduler_counter = 0;
// 	process->in_scheduler = 0;
// 	process->waiting_pid = -1;
// 	process->parent_pid = -1;
// 	process->pid = -1;
// 	process->priority = 0;
// 	process->base_priority = 0;
// 	process->saved_priority = 0;
// 	process->foreground_priority_boost = 0;
// 	process->waiting_ticks = 0;
// 	process->pending_cleanup = 0;
// 	process->has_foreground = 0;

// 	if (pending_cleanup_process == process) {
// 		pending_cleanup_process = NULL;
// 	}

// 	reset_process_name(process);
// }

//////////////////////////////////////////////////////////////////////////////////////
// static void release_process_resources(PCB *process) {
// 	if (process == NULL) {
// 		return;
// 	}

// 	// Sacarlo del scheduler si está
// 	removeFromScheduler(process);

// 	// Libero argv / argc
// 	release_process_arguments(process);

// 	// Libero recursos de IO (pipes, etc.)
// 	release_process_io_resources(process);

// 	// Libero stack si existe
// 	if (process->stack_base != NULL && memory_manager != NULL) {
// 		uint64_t aligned = (uint64_t) process->stack_base;

// 		// El puntero "real" devuelto por memory_alloc está guardado
// 		// inmediatamente antes del stack alineado
// 		void **slot = (void **) (aligned - sizeof(void *));
// 		void *raw  = *slot;

// 		if (raw != NULL) {
// 			memory_free(memory_manager, raw);
// 		}

// 		process->stack_base = NULL;
// 		process->stack_pointer = NULL;
// 	}

// 	// Resetear campos del PCB
// 	process->entry_point = NULL;
// 	process->in_scheduler = 0;
// 	process->waiting_pid = -1;
// 	process->parent_pid = -1;
// 	process->pid = -1;
// 	process->priority = 0;
// 	process->base_priority = 0;
// 	process->saved_priority = 0;
// 	process->foreground_priority_boost = 0;
// 	process->scheduler_counter = 0;
// 	process->waiting_ticks = 0;
// 	process->pending_cleanup = 0;
// 	process->has_foreground = 0;

// 	if (pending_cleanup_process == process) {
// 		pending_cleanup_process = NULL;
// 	}

// 	reset_process_name(process);
// 	process->state = TERMINATED;
// }

static void release_process_resources(PCB *process) {
	if (process == NULL) {
		return;
	}

	// Sacarlo del scheduler
	removeFromScheduler(process);

	// Libero argv / argc
	release_process_arguments(process);

	// Libero recursos de IO (pipes, etc.)
	release_process_io_resources(process);

	// *** NO liberar stack por ahora ***
	// if (process->stack_base != NULL && memory_manager != NULL) {
	//     uint64_t aligned = (uint64_t) process->stack_base;
	//     void **slot = (void **) (aligned - sizeof(void *));
	//     void *raw  = *slot;
	//     if (raw != NULL) {
	//         memory_free(memory_manager, raw);
	//     }
	// }
	process->stack_base = NULL;
	process->stack_pointer = NULL;

	// Resetear campos del PCB
	process->entry_point = NULL;
	process->in_scheduler = 0;
	process->waiting_pid = -1;
	process->parent_pid = -1;
	process->pid = -1;
	process->priority = 0;
	process->base_priority = 0;
	process->saved_priority = 0;
	process->foreground_priority_boost = 0;
	process->scheduler_counter = 0;
	process->waiting_ticks = 0;
	process->pending_cleanup = 0;
	process->has_foreground = 0;

	if (pending_cleanup_process == process) {
		pending_cleanup_process = NULL;
	}

	reset_process_name(process);
	process->state = TERMINATED;
}

///////////////////////////////////////////////////

// static int duplicate_arguments(PCB *process, int argc, char **argv) {
// 	if (process == NULL) {
// 		return -1;
// 	}

// 	process->argc = 0;
// 	process->argv = NULL;

// 	if (argc <= 0 || argv == NULL) {
// 		return 0;
// 	}

// 	process->argv = (char **) memory_alloc(memory_manager, (argc + 1) * sizeof(char *));
// 	if (process->argv == NULL) {
// 		return -1;
// 	}

// 	for (int i = 0; i < argc; i++) {
// 		process->argv[i] = NULL;
// 	}

// 	for (int i = 0; i < argc; i++) {
// 		if (argv[i] == NULL) {
// 			continue;
// 		}

// 		int len = 0;
// 		while (argv[i][len] != '\0') {
// 			len++;
// 		}

// 		process->argv[i] = (char *) memory_alloc(memory_manager, len + 1);
// 		if (process->argv[i] == NULL) {
// 			release_process_arguments(process);
// 			return -1;
// 		}

// 		for (int j = 0; j <= len; j++) {
// 			process->argv[i][j] = argv[i][j];
// 		}
// 	}

// 	process->argv[argc] = NULL;
// 	process->argc = argc;

// 	return 0;
// }

//////////////////////////////////////////////////////////////////////////
// int duplicate_arguments(PCB *p, int argc, char **argv) {
// 	p->argc = 0;
// 	p->argv = NULL;

// 	if (argc <= 0 || argv == NULL) {
// 		return 0;
// 	}

// 	char **copy = memory_alloc(memory_manager, sizeof(char *) * (argc + 1));
// 	if (copy == NULL) {
// 		return -1;
// 	}

// 	for (int i = 0; i < argc; i++) {
// 		if (argv[i] == NULL) {
// 			copy[i] = NULL;
// 			continue;
// 		}

// 		int len = 0;
// 		while (argv[i][len] != '\0')
// 			len++;

// 		copy[i] = memory_alloc(memory_manager, len + 1);
// 		if (copy[i] == NULL) {
// 			for (int j = 0; j < i; j++) {
// 				if (copy[j] != NULL)
// 					memory_free(memory_manager, copy[j]);
// 			}
// 			memory_free(memory_manager, copy);
// 			return -1;
// 		}

// 		for (int k = 0; k < len; k++) {
// 			copy[i][k] = argv[i][k];
// 		}
// 		copy[i][len] = '\0';
// 	}

// 	copy[argc] = NULL;

// 	p->argc = argc;
// 	p->argv = copy;
// 	return 0;
// }

static int duplicate_arguments(PCB *process, int argc, char **argv) {
	if (process == NULL)
		return -1;

	process->argc = 0;
	process->argv = NULL;

	if (argc <= 0 || argv == NULL)
		return 0;

	/* Crear vector de punteros */
	process->argv = (char **) memory_alloc(memory_manager, (argc + 1) * sizeof(char *));
	if (process->argv == NULL)
		return -1;

	for (int i = 0; i < argc; i++)
		process->argv[i] = NULL;

	for (int i = 0; i < argc; i++) {
		if (argv[i] == NULL) {
			process->argv[i] = NULL;
			continue;
		}

		/* Calcular longitud segura */
		int len = 0;
		while (argv[i][len] != 0)
			len++;

		/* Asignar */
		process->argv[i] = (char *) memory_alloc(memory_manager, len + 1);
		if (process->argv[i] == NULL) {
			release_process_arguments(process);
			return -1;
		}

		/* Copiar sin pasarse */
		for (int j = 0; j < len; j++)
			process->argv[i][j] = argv[i][j];

		process->argv[i][len] = 0; /* NULL-terminate */
	}

	process->argv[argc] = NULL;
	process->argc = argc;

	return 0;
}

////////////////////////////////////////////////////////////////

static void init_process_io_defaults(ProcessIOState *io_state) {
	if (io_state == NULL) {
		return;
	}

	io_state->stdin_desc.type = IO_SOURCE_KEYBOARD;
	io_state->stdin_desc.resource_id = PROCESS_IO_RESOURCE_INVALID;
	io_state->stdout_desc.type = IO_SINK_SCREEN;
	io_state->stdout_desc.resource_id = PROCESS_IO_RESOURCE_INVALID;
	io_state->stderr_desc.type = IO_SINK_SCREEN;
	io_state->stderr_desc.resource_id = PROCESS_IO_RESOURCE_INVALID;
	io_state->stdin_eof = 0;
	io_state->stdin_echo = 1; // Por defecto, activar echo para teclado
}

static int retain_process_io_resources(PCB *process) {
	if (process == NULL) {
		return -1;
	}

	int stdin_registered = 0;
	int stdout_registered = 0;
	int stderr_registered = 0;

	if (process->io_state.stdin_desc.type == IO_SOURCE_PIPE && process->io_state.stdin_desc.resource_id >= 0) {
		if (pipe_register_reader(process->io_state.stdin_desc.resource_id) != 0) {
			return -1;
		}
		stdin_registered = 1;
	}

	if (process->io_state.stdout_desc.type == IO_SINK_PIPE && process->io_state.stdout_desc.resource_id >= 0) {
		if (pipe_register_writer(process->io_state.stdout_desc.resource_id) != 0) {
			if (stdin_registered) {
				pipe_unregister_reader(process->io_state.stdin_desc.resource_id);
			}
			return -1;
		}
		stdout_registered = 1;
	}

	if (process->io_state.stderr_desc.type == IO_SINK_PIPE && process->io_state.stderr_desc.resource_id >= 0) {
		if (pipe_register_writer(process->io_state.stderr_desc.resource_id) != 0) {
			if (stdout_registered) {
				pipe_unregister_writer(process->io_state.stdout_desc.resource_id);
			}
			if (stdin_registered) {
				pipe_unregister_reader(process->io_state.stdin_desc.resource_id);
			}
			return -1;
		}
		stderr_registered = 1;
	}

	return 0;
}

static void release_process_io_resources(PCB *process) {
	if (process == NULL) {
		return;
	}

	if (process->io_state.stdin_desc.type == IO_SOURCE_PIPE && process->io_state.stdin_desc.resource_id >= 0) {
		pipe_unregister_reader(process->io_state.stdin_desc.resource_id);
	}

	if (process->io_state.stdout_desc.type == IO_SINK_PIPE && process->io_state.stdout_desc.resource_id >= 0) {
		pipe_unregister_writer(process->io_state.stdout_desc.resource_id);
	}

	if (process->io_state.stderr_desc.type == IO_SINK_PIPE && process->io_state.stderr_desc.resource_id >= 0) {
		pipe_unregister_writer(process->io_state.stderr_desc.resource_id);
	}

	init_process_io_defaults(&process->io_state);
}

static int set_process_stdin_descriptor(PCB *process, uint32_t type, int32_t resource) {
	if (process == NULL) {
		return -1;
	}

	if (type == PROCESS_IO_STDIN_INHERIT) {
		return 0;
	}

	ProcessInputDescriptor previous = process->io_state.stdin_desc;
#if PIPE_DEBUG
	vd_print("[PROC] set_stdin pid=");
	vd_print_dec(process->pid);
	vd_print(" type=");
	vd_print_dec(type);
	vd_print(" resource=");
	vd_print_dec(resource);
	vd_print("\n");
#endif

	if (type == PROCESS_IO_STDIN_KEYBOARD) {
		if (previous.type == IO_SOURCE_PIPE && previous.resource_id >= 0) {
			pipe_unregister_reader(previous.resource_id);
		}
		process->io_state.stdin_desc.type = IO_SOURCE_KEYBOARD;
		process->io_state.stdin_desc.resource_id = PROCESS_IO_RESOURCE_INVALID;
		process->io_state.stdin_echo = 1; // Activar echo cuando se lee del teclado
		return 0;
	}

	if (type == PROCESS_IO_STDIN_PIPE) {
		if (resource < 0) {
			return -1;
		}

		if (previous.type == IO_SOURCE_PIPE && previous.resource_id == resource) {
			return 0;
		}

		if (previous.type == IO_SOURCE_PIPE && previous.resource_id >= 0) {
			pipe_unregister_reader(previous.resource_id);
		}

		if (pipe_register_reader(resource) != 0) {
#if PIPE_DEBUG
			vd_print("[PROC] ERROR: pipe_register_reader failed pid=");
			vd_print_dec(process->pid);
			vd_print(" pipe_id=");
			vd_print_dec(resource);
			vd_print("\n");
#endif
			if (previous.type == IO_SOURCE_PIPE && previous.resource_id >= 0) {
				pipe_register_reader(previous.resource_id);
			}
			process->io_state.stdin_desc = previous;
			return -1;
		}

		process->io_state.stdin_desc.type = IO_SOURCE_PIPE;
		process->io_state.stdin_desc.resource_id = resource;
		process->io_state.stdin_echo = 0; // Desactivar echo cuando se lee de pipe (el escritor ya hizo echo)
#if PIPE_DEBUG
		vd_print("[PROC] Successfully set stdin to PIPE pid=");
		vd_print_dec(process->pid);
		vd_print(" pipe_id=");
		vd_print_dec(resource);
		vd_print("\n");
#endif
		return 0;
	}

	return -1;
}

static int set_process_stdout_descriptor(PCB *process, uint32_t type, int32_t resource) {
	if (process == NULL) {
		return -1;
	}

	if (type == PROCESS_IO_STDOUT_INHERIT) {
		return 0;
	}

	ProcessOutputDescriptor previous = process->io_state.stdout_desc;
#if PIPE_DEBUG
	vd_print("[PROC] set_stdout pid=");
	vd_print_dec(process->pid);
	vd_print(" type=");
	vd_print_dec(type);
	vd_print(" resource=");
	vd_print_dec(resource);
	vd_print("\n");
#endif

	if (type == PROCESS_IO_STDOUT_SCREEN) {
		if (previous.type == IO_SINK_PIPE && previous.resource_id >= 0) {
			pipe_unregister_writer(previous.resource_id);
		}
		process->io_state.stdout_desc.type = IO_SINK_SCREEN;
		process->io_state.stdout_desc.resource_id = PROCESS_IO_RESOURCE_INVALID;
		return 0;
	}

	if (type == PROCESS_IO_STDOUT_PIPE) {
		if (resource < 0) {
			return -1;
		}

		if (previous.type == IO_SINK_PIPE && previous.resource_id == resource) {
			return 0;
		}

		if (previous.type == IO_SINK_PIPE && previous.resource_id >= 0) {
			pipe_unregister_writer(previous.resource_id);
		}

		if (pipe_register_writer(resource) != 0) {
#if PIPE_DEBUG
			vd_print("[PROC] ERROR: pipe_register_writer failed pid=");
			vd_print_dec(process->pid);
			vd_print(" pipe_id=");
			vd_print_dec(resource);
			vd_print("\n");
#endif
			if (previous.type == IO_SINK_PIPE && previous.resource_id >= 0) {
				pipe_register_writer(previous.resource_id);
			}
			process->io_state.stdout_desc = previous;
			return -1;
		}

		process->io_state.stdout_desc.type = IO_SINK_PIPE;
		process->io_state.stdout_desc.resource_id = resource;
#if PIPE_DEBUG
		vd_print("[PROC] Successfully set stdout to PIPE pid=");
		vd_print_dec(process->pid);
		vd_print(" pipe_id=");
		vd_print_dec(resource);
		vd_print("\n");
#endif
		return 0;
	}

	return -1;
}

static int set_process_stderr_descriptor(PCB *process, uint32_t type, int32_t resource) {
	if (process == NULL) {
		return -1;
	}

	if (type == PROCESS_IO_STDERR_INHERIT) {
		return 0;
	}

	ProcessOutputDescriptor previous = process->io_state.stderr_desc;

	if (type == PROCESS_IO_STDERR_SCREEN) {
		if (previous.type == IO_SINK_PIPE && previous.resource_id >= 0) {
			pipe_unregister_writer(previous.resource_id);
		}
		process->io_state.stderr_desc.type = IO_SINK_SCREEN;
		process->io_state.stderr_desc.resource_id = PROCESS_IO_RESOURCE_INVALID;
		return 0;
	}

	if (type == PROCESS_IO_STDERR_PIPE) {
		if (resource < 0) {
			return -1;
		}

		if (previous.type == IO_SINK_PIPE && previous.resource_id == resource) {
			return 0;
		}

		if (previous.type == IO_SINK_PIPE && previous.resource_id >= 0) {
			pipe_unregister_writer(previous.resource_id);
		}

		if (pipe_register_writer(resource) != 0) {
			if (previous.type == IO_SINK_PIPE && previous.resource_id >= 0) {
				pipe_register_writer(previous.resource_id);
			}
			process->io_state.stderr_desc = previous;
			return -1;
		}

		process->io_state.stderr_desc.type = IO_SINK_PIPE;
		process->io_state.stderr_desc.resource_id = resource;
		return 0;
	}

	return -1;
}

static int override_process_io(PCB *process, const process_io_config_t *config) {
	if (process == NULL || config == NULL) {
		return 0;
	}

	if (set_process_stdin_descriptor(process, config->stdin_type, config->stdin_resource) != 0) {
		return -1;
	}

	if (set_process_stdout_descriptor(process, config->stdout_type, config->stdout_resource) != 0) {
		return -1;
	}

	if (set_process_stderr_descriptor(process, config->stderr_type, config->stderr_resource) != 0) {
		return -1;
	}

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
		init_process_io_defaults(&process_table[i].io_state);
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
	init_process_io_defaults(&idle_pcb->io_state);
	const char *idle_name = "idle";
	int i = 0;
	while (idle_name[i] != '\0' && i < MAX_PROCESS_NAME - 1) {
		idle_pcb->name[i] = idle_name[i];
		i++;
	}
	idle_pcb->name[i] = '\0';
	idle_pcb->entry_point = NULL; // El proceso idle no tiene entry_point userland
	// Asignar stack con espacio extra para alineacion y puntero original (consistente con create_process_internal)
	void *idle_stack_allocated = memory_alloc(memory_manager, STACK_SIZE + 16 + sizeof(void *));
	if (idle_stack_allocated == NULL) {
		return -1;
	}
	// Alinear stack_base a 16 bytes desde el principio, dejando espacio para el puntero
	uint64_t idle_stack_base_aligned = ((uint64_t) idle_stack_allocated + sizeof(void *) + 15) & ~((uint64_t) 0xF);
	// Guardar el puntero original justo antes del stack_base alineado
	*((void **) (idle_stack_base_aligned - sizeof(void *))) = idle_stack_allocated;
	idle_pcb->stack_base = (void *) idle_stack_base_aligned;

	uint64_t idle_stack_limit = idle_stack_base_aligned + STACK_SIZE;
	uint64_t idle_user_rsp = (idle_stack_limit - USER_STACK_GUARD_SIZE) & ~((uint64_t) 0xF);
	void *idle_frame_pointer = (void *) idle_user_rsp;
	idle_pcb->stack_pointer = set_process_stack(0, NULL, idle_frame_pointer, (void *) idle_pcb);
	idle_pcb->parent_pid = -1; // Idle no tiene padre

	current_process = idle_pcb;
	current_process->state = RUNNING;
	init_ready_queue();
	pending_cleanup_process = NULL;

	// Inicializar pipes antes de marcar como inicializado
	init_pipes();

	initialized_flag = 1;
	return 0;
}

int processes_initialized() {
	return initialized_flag;
}
///////////////////////////////////////////////////////
// static process_id_t create_process_internal(
// 	const char *name,
// 	void (*entry_point)(int, char **),
// 	int argc, char **argv,
// 	uint8_t priority,
// 	const process_io_config_t *io_config,
// 	int assign_foreground
// ) {
// 	if (!initialized_flag || entry_point == NULL) {
// 		return -1;
// 	}

// 	if (priority > MAX_PRIORITY) {
// 		priority = MAX_PRIORITY;
// 	}

// 	// Limpieza agresiva antes de reservar cosas nuevas
// 	free_terminated_processes();

// 	// Buscar slot libre (salteamos 0 porque es idle)
// 	int slot = -1;
// 	for (int i = 1; i < MAX_PROCESSES; i++) {
// 		if (process_table[i].state == TERMINATED) {
// 			slot = i;
// 			break;
// 		}
// 	}
// 	if (slot == -1) {
// 		return -1;
// 	}

// 	PCB *p = &process_table[slot];

// 	// Si el slot todavía tiene recursos viejos, limpiarlos
// 	if (p->stack_base != NULL || p->argv != NULL) {
// 		release_process_resources(p);
// 	}

// 	/* ============================
// 	   Inicialización crítica del PCB
// 	   ============================ */
// 	p->entry_point = entry_point;   // ¡RIP válido desde el inicio!
// 	p->argc = 0;
// 	p->argv = NULL;
// 	p->stack_base = NULL;
// 	p->stack_pointer = NULL;
// 	p->in_scheduler = 0;
// 	p->waiting_pid = -1;

// 	p->pid = get_next_valid_pid();
// 	p->state = READY;
// 	p->priority = priority;
// 	p->base_priority = priority;
// 	p->saved_priority = priority;
// 	p->foreground_priority_boost = 0;
// 	p->scheduler_counter = 0;
// 	p->waiting_ticks = 0;
// 	p->pending_cleanup = 0;
// 	p->parent_pid = get_current_pid();
// 	p->has_foreground = 0;

// 	init_process_io_defaults(&p->io_state);

// 	// Copiar nombre
// 	if (name != NULL) {
// 		int i = 0;
// 		while (name[i] != '\0' && i < MAX_PROCESS_NAME - 1) {
// 			p->name[i] = name[i];
// 			i++;
// 		}
// 		p->name[i] = '\0';
// 	} else {
// 		p->name[0] = '?';
// 		p->name[1] = '\0';
// 	}

// 	/* ============================
// 	   Herencia / override de IO
// 	   ============================ */
// 	PCB *parent = get_process_by_pid(p->parent_pid);
// 	if (parent != NULL) {
// 		p->io_state = parent->io_state;
// 		p->io_state.stdin_eof = 0;

// 		if (io_config == NULL) {
// 			if (retain_process_io_resources(p) != 0) {
// 				init_process_io_defaults(&p->io_state);
// 				p->state = TERMINATED;
// 				return -1;
// 			}
// 		}
// 	} else {
// 		init_process_io_defaults(&p->io_state);
// 	}

// 	if (io_config != NULL) {
// 		ProcessIOState prev = p->io_state;

// 		if (override_process_io(p, io_config) != 0) {
// 			release_process_io_resources(p);
// 			p->state = TERMINATED;
// 			return -1;
// 		}

// 		// Registrar pipes heredados no modificados
// 		if (prev.stdin_desc.type == IO_SOURCE_PIPE &&
// 			p->io_state.stdin_desc.type == IO_SOURCE_PIPE &&
// 			prev.stdin_desc.resource_id == p->io_state.stdin_desc.resource_id &&
// 			p->io_state.stdin_desc.resource_id >= 0) {

// 			if (pipe_register_reader(p->io_state.stdin_desc.resource_id) != 0) {
// 				release_process_io_resources(p);
// 				p->state = TERMINATED;
// 				return -1;
// 			}
// 		}

// 		if (prev.stdout_desc.type == IO_SINK_PIPE &&
// 			p->io_state.stdout_desc.type == IO_SINK_PIPE &&
// 			prev.stdout_desc.resource_id == p->io_state.stdout_desc.resource_id &&
// 			p->io_state.stdout_desc.resource_id >= 0) {

// 			if (pipe_register_writer(p->io_state.stdout_desc.resource_id) != 0) {
// 				release_process_io_resources(p);
// 				p->state = TERMINATED;
// 				return -1;
// 			}
// 		}

// 		if (prev.stderr_desc.type == IO_SINK_PIPE &&
// 			p->io_state.stderr_desc.type == IO_SINK_PIPE &&
// 			prev.stderr_desc.resource_id == p->io_state.stderr_desc.resource_id &&
// 			p->io_state.stderr_desc.resource_id >= 0) {

// 			if (pipe_register_writer(p->io_state.stderr_desc.resource_id) != 0) {
// 				release_process_io_resources(p);
// 				p->state = TERMINATED;
// 				return -1;
// 			}
// 		}
// 	}

// 	/* ============================
// 	   Duplicar argumentos
// 	   ============================ */
// 	if (duplicate_arguments(p, argc, argv) < 0) {
// 		release_process_io_resources(p);
// 		p->state = TERMINATED;
// 		return -1;
// 	}

// 	// Otra limpieza antes de reservar stack (por si liberó algo)
// 	free_terminated_processes();

// 	/* ============================
// 	   Reservar y preparar STACK
// 	   ============================ */

// 	// Reservamos un poco de extra para alineación + puntero original
// 	void *raw = memory_alloc(memory_manager, STACK_SIZE + 16 + sizeof(void *));
// 	if (raw == NULL) {
// 		release_process_arguments(p);
// 		release_process_io_resources(p);
// 		p->state = TERMINATED;
// 		return -1;
// 	}

// 	// Alinear a 16 bytes dejando espacio para guardar raw
// 	uint64_t aligned = ((uint64_t) raw + sizeof(void *) + 15) & ~0xFULL;

// 	// Guardar puntero original inmediatamente antes del stack_base
// 	*((void **) (aligned - sizeof(void *))) = raw;

// 	p->stack_base = (void *) aligned;

// 	// RSP inicial: al final del área válida, alineado
// 	uint64_t rsp = aligned + STACK_SIZE - 16;
// 	rsp &= ~0xFULL;
// 	void *stack_top = (void *) rsp;

// 	// Construir contexto inicial
// 	p->stack_pointer = set_process_stack(p->argc, p->argv, stack_top, (void *) p);

// 	if (p->stack_pointer == NULL) {
// 		// Error real armando el contexto: liberar todo
// 		memory_free(memory_manager, raw);
// 		release_process_arguments(p);
// 		release_process_io_resources(p);
// 		p->stack_base = NULL;
// 		p->state = TERMINATED;
// 		return -1;
// 	}

// 	/* ============================
// 	   Agregar al scheduler y foreground
// 	   ============================ */

// 	if (addToScheduler(p) < 0) {
// 		// No lo pudimos encolar: liberar recursos
// 		memory_free(memory_manager, raw);
// 		release_process_arguments(p);
// 		release_process_io_resources(p);
// 		p->stack_base = NULL;
// 		p->stack_pointer = NULL;
// 		p->state = TERMINATED;
// 		return -1;
// 	}

// 	if (assign_foreground) {
// 		if (set_foreground_process(p->pid) < 0) {
// 			terminate_process(p, 1);
// 			return -1;
// 		}
// 	}

// 	return p->pid;
// }

//////////////////////////////////////////////7
static process_id_t create_process_internal(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
											uint8_t priority, const process_io_config_t *io_config,
											int assign_foreground) {
	if (!initialized_flag || entry_point == NULL) {
		return -1;
	}

	if (priority > MAX_PRIORITY) {
		priority = MAX_PRIORITY;
	}

	// Intentar limpiar procesos muertos antes de buscar slot
	free_terminated_processes();

	/* ============================
	   BUSCAR SLOT LIBRE
	   ============================ */
	int slot = -1;
	for (int i = 1; i < MAX_PROCESSES; i++) { // 0 es idle
		if (process_table[i].state == TERMINATED) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		return -1;
	}

	PCB *p = &process_table[slot];

	// Si tenía cosas de antes, limpiarlas
	if (p->stack_base != NULL || p->argv != NULL) {
		release_process_resources(p);
	}

	/* ============================
	   INICIALIZACIÓN BÁSICA DEL PCB
	   ============================ */
	p->entry_point = entry_point;
	p->argc = 0;
	p->argv = NULL;
	p->stack_base = NULL;
	p->stack_pointer = NULL;
	p->in_scheduler = 0;
	p->waiting_pid = -1;

	p->pid = get_next_valid_pid();
	p->state = READY;
	p->priority = priority;
	p->base_priority = priority;
	p->saved_priority = priority;
	p->foreground_priority_boost = 0;
	p->scheduler_counter = 0;
	p->waiting_ticks = 0;
	p->pending_cleanup = 0;
	p->parent_pid = get_current_pid();
	p->has_foreground = 0;

	init_process_io_defaults(&p->io_state);

	// Nombre
	if (name != NULL) {
		int i = 0;
		while (name[i] != '\0' && i < MAX_PROCESS_NAME - 1) {
			p->name[i] = name[i];
			i++;
		}
		p->name[i] = '\0';
	}
	else {
		p->name[0] = '?';
		p->name[1] = '\0';
	}

	/* ============================
	   HERENCIA / OVERRIDE DE IO
	   ============================ */
	PCB *parent = get_process_by_pid(p->parent_pid);
	if (parent != NULL) {
		p->io_state = parent->io_state;
		p->io_state.stdin_eof = 0;

		if (io_config == NULL) {
			// Registrar recursos heredados si usa pipes
			if (retain_process_io_resources(p) != 0) {
				init_process_io_defaults(&p->io_state);
				p->state = TERMINATED;
				return -1;
			}
		}
	}
	else {
		init_process_io_defaults(&p->io_state);
	}

	if (io_config != NULL) {
		ProcessIOState prev = p->io_state;

		if (override_process_io(p, io_config) != 0) {
			release_process_io_resources(p);
			p->state = TERMINATED;
			return -1;
		}

		// Registrar pipes heredados no modificados
		if (prev.stdin_desc.type == IO_SOURCE_PIPE && p->io_state.stdin_desc.type == IO_SOURCE_PIPE &&
			prev.stdin_desc.resource_id == p->io_state.stdin_desc.resource_id &&
			p->io_state.stdin_desc.resource_id >= 0) {
			if (pipe_register_reader(p->io_state.stdin_desc.resource_id) != 0) {
				release_process_io_resources(p);
				p->state = TERMINATED;
				return -1;
			}
		}

		if (prev.stdout_desc.type == IO_SINK_PIPE && p->io_state.stdout_desc.type == IO_SINK_PIPE &&
			prev.stdout_desc.resource_id == p->io_state.stdout_desc.resource_id &&
			p->io_state.stdout_desc.resource_id >= 0) {
			if (pipe_register_writer(p->io_state.stdout_desc.resource_id) != 0) {
				release_process_io_resources(p);
				p->state = TERMINATED;
				return -1;
			}
		}

		if (prev.stderr_desc.type == IO_SINK_PIPE && p->io_state.stderr_desc.type == IO_SINK_PIPE &&
			prev.stderr_desc.resource_id == p->io_state.stderr_desc.resource_id &&
			p->io_state.stderr_desc.resource_id >= 0) {
			if (pipe_register_writer(p->io_state.stderr_desc.resource_id) != 0) {
				release_process_io_resources(p);
				p->state = TERMINATED;
				return -1;
			}
		}
	}

	/* ============================
	   COPIA DE ARGUMENTOS
	   ============================ */
	if (duplicate_arguments(p, (int) argc, argv) < 0) {
		release_process_io_resources(p);
		p->state = TERMINATED;
		return -1;
	}

	// Otra pasada de limpieza agresiva antes de pedir stack
	free_terminated_processes();

	/* ============================
	   ASIGNAR STACK
	   ============================ */
	void *raw = memory_alloc(memory_manager, STACK_SIZE + 16 + sizeof(void *));
	if (raw == NULL) {
		release_process_arguments(p);
		release_process_io_resources(p);
		p->state = TERMINATED;
		return -1;
	}

	// Alinear a 16 bytes dejando espacio para guardar puntero original
	uint64_t aligned = ((uint64_t) raw + sizeof(void *) + 15) & ~0xFULL;
	*((void **) (aligned - sizeof(void *))) = raw;
	p->stack_base = (void *) aligned;

	uint64_t stack_limit = aligned + STACK_SIZE;
	uint64_t rsp = (stack_limit - USER_STACK_GUARD_SIZE) & ~0xFULL;
	void *stack_top = (void *) rsp;

	p->stack_pointer = set_process_stack(p->argc, p->argv, stack_top, (void *) p);

	if (p->stack_pointer == NULL) {
		// Error real armando contexto: liberamos el raw MANUALMENTE
		memory_free(memory_manager, raw);
		p->stack_base = NULL;
		p->stack_pointer = NULL;
		release_process_arguments(p);
		release_process_io_resources(p);
		p->state = TERMINATED;
		return -1;
	}

	/* ============================
	   AGREGAR AL SCHEDULER
	   ============================ */
	if (addToScheduler(p) < 0) {
		// En este caso usamos la función de release que libera también el stack
		release_process_resources(p);
		p->state = TERMINATED;
		return -1;
	}

	/* ============================
	   FOREFROUND (opcional)
	   ============================ */
	if (assign_foreground) {
		if (set_foreground_process(p->pid) < 0) {
			terminate_process(p, 1);
			return -1;
		}
	}

	return p->pid;
}

process_id_t create_process(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
							uint8_t priority) {
	return create_process_internal(name, entry_point, argc, argv, priority, NULL, 0);
}

process_id_t create_process_with_io(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
									uint8_t priority, const process_io_config_t *io_config) {
	return create_process_internal(name, entry_point, argc, argv, priority, io_config, 0);
}

process_id_t create_process_foreground_with_io(const char *name, void (*entry_point)(int, char **), int argc,
											   char **argv, uint8_t priority, const process_io_config_t *io_config) {
	return create_process_internal(name, entry_point, argc, argv, priority, io_config, 1);
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

int process_read(process_id_t pid, int fd, char *buffer, size_t count) {
	if (fd != STDIN || buffer == NULL) {
		return -1;
	}

	if (count == 0) {
		return 0;
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return -1;
	}

	if (process->io_state.stdin_eof) {
		return 0;
	}

	int bytes_to_read = (int) count;
	if (process->io_state.stdin_desc.type == IO_SOURCE_KEYBOARD) {
		int result = read_keyboard(buffer, bytes_to_read);
		// El terminal nunca debe marcar su stdin como EOF
		// Solo marcar EOF si no es el terminal (shell/shellCreated)
		if (result == 0 && process != NULL) {
			int is_terminal = (kstrcmp(process->name, "shell") == 0 || kstrcmp(process->name, "shellCreated") == 0);
			if (!is_terminal) {
				process->io_state.stdin_eof = 1;
			}
		}
		return result;
	}

	if (process->io_state.stdin_desc.type == IO_SOURCE_PIPE && process->io_state.stdin_desc.resource_id >= 0) {
#if PIPE_DEBUG
		vd_print("[PROC] process_read from PIPE pid=");
		vd_print_dec(process->pid);
		vd_print(" pipe_id=");
		vd_print_dec(process->io_state.stdin_desc.resource_id);
		vd_print(" count=");
		vd_print_dec(count);
		vd_print("\n");
#endif
		int result = pipe_read(process->io_state.stdin_desc.resource_id, buffer, count);
		// pipe_read deberia bloquearse si no hay datos disponibles
		// Si retorna -1, es un error critico (semaforo no existe, pipe invalido, etc.)
		// Si retorna > 0, leyo datos correctamente
		// Si retorna 0, es EOF (no hay escritores y buffer vacio)
		if (result < 0) {
			// Error critico al leer del pipe
			// Si no hay escritores, marcar EOF
			if (!pipe_has_writers(process->io_state.stdin_desc.resource_id)) {
				process->io_state.stdin_eof = 1;
				return 0; // Retornar 0 para indicar EOF
			}
			// Hay escritores pero hubo error critico
			// Esto indica un problema de configuracion (semaforo no existe, etc.)
			// Retornar -1 para indicar error
			return -1;
		}
		if (result == 0) {
			// EOF: no hay mas datos y no hay escritores
			// Si el proceso es foreground, cambiar stdin a teclado para permitir lectura interactiva
			// Esto permite que comandos como cat puedan leer del teclado despues de leer del pipe
			if (process->has_foreground && foregroundProcessPid == process->pid) {
				// Guardar el pipe_id antes de cambiarlo
				int pipe_id = process->io_state.stdin_desc.resource_id;
				// Desregistrar del pipe como lector
				pipe_unregister_reader(pipe_id);
				// Cambiar stdin de pipe a teclado
				process->io_state.stdin_desc.type = IO_SOURCE_KEYBOARD;
				process->io_state.stdin_desc.resource_id = PROCESS_IO_RESOURCE_INVALID;
				// Resetear EOF para permitir lectura del teclado
				process->io_state.stdin_eof = 0;
				// Intentar leer del teclado ahora
				return read_keyboard(buffer, bytes_to_read);
			}
			// Si no es foreground, marcar EOF normalmente
			process->io_state.stdin_eof = 1;
			return 0;
		}
		// result > 0, leer correctamente
		// Resetear EOF flag si leimos datos (por si se marco antes por error)
		process->io_state.stdin_eof = 0;
		return result;
	}

	// Descriptor de stdin no configurado o tipo invalido
	// Esto no deberia pasar si el proceso fue creado correctamente
	return -1;
}

int process_write(process_id_t pid, int fd, const char *buffer, size_t count, size_t color, size_t background) {
	if ((buffer == NULL && count != 0)) {
		return -1;
	}

	if (count == 0) {
		return 0;
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return -1;
	}

	int bytes_to_write = (int) count;

	if (fd == STDOUT) {
		if (process->io_state.stdout_desc.type == IO_SINK_SCREEN) {
			write(buffer, bytes_to_write, color, background);
			return bytes_to_write;
		}
		if (process->io_state.stdout_desc.type == IO_SINK_PIPE && process->io_state.stdout_desc.resource_id >= 0) {
#if PIPE_DEBUG
			vd_print("[PROC] process_write to PIPE pid=");
			vd_print_dec(process->pid);
			vd_print(" pipe_id=");
			vd_print_dec(process->io_state.stdout_desc.resource_id);
			vd_print(" count=");
			vd_print_dec(count);
			vd_print("\n");
#endif
			int result = pipe_write(process->io_state.stdout_desc.resource_id, buffer, count);
			// pipe_write retorna -1 en error, >= 0 en exito
			return result;
		}
		// Descriptor de stdout no configurado o tipo invalido
		return -1;
	}

	if (fd == STDERR) {
		if (process->io_state.stderr_desc.type == IO_SINK_SCREEN) {
			write(buffer, bytes_to_write, 0x00ff0000, background);
			return bytes_to_write;
		}
		if (process->io_state.stderr_desc.type == IO_SINK_PIPE && process->io_state.stderr_desc.resource_id >= 0) {
			return pipe_write(process->io_state.stderr_desc.resource_id, buffer, count);
		}
		// Descriptor de stderr no configurado o tipo invalido
		return -1;
	}

	// File descriptor invalido (no es STDIN, STDOUT ni STDERR)
	return -1;
}

PCB *get_process_by_pid(process_id_t pid) {
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

	// Remover el proceso de todas las colas de semaforos
	sem_remove_process(pid);

	// SEMANTICA CORRECTA: waiting_pid = PID del proceso que esta esperando a este proceso
	// Si hay un proceso esperando a este proceso, despertarlo
	if (process->waiting_pid != -1) {
		PCB *waiter = get_process_by_pid(process->waiting_pid);
		if (waiter != NULL && waiter->state == BLOCKED) {
			waiter->state = READY;
			waiter->waiting_pid = -1;
			addToScheduler(waiter);
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

	// if (foregroundProcessPid == pid) {
	// 	release_foreground_owner();
	// }

	////////////TODO: Ver esto
	/* Si este proceso tenía el foreground, devolverlo a su padre */
	if (foregroundProcessPid == pid) {
		release_foreground_owner();

		PCB *parent = get_process_by_pid(process->parent_pid);
		if (parent != NULL && parent->state != TERMINATED) {
			foregroundProcessPid = parent->pid;
			parent->has_foreground = 1;

			/* Reestablecer prioridad normal */
			if (!parent->foreground_priority_boost) {
				parent->saved_priority = parent->priority;
				parent->priority = MIN_PRIORITY;
				parent->foreground_priority_boost = 1;
			}
		}
	}

	///////////////

	release_process_io_resources(process);

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

	PCB *current = get_process_by_pid(get_current_pid());
	if (current == NULL) {
		return -1;
	}

	PCB *child_process = get_process_by_pid(pid);
	if (child_process == NULL) {
		return -1;
	}

	// SEMANTICA CORRECTA: waiting_pid = PID del proceso que esta esperando a este proceso
	// Si proceso A espera a proceso B, entonces B->waiting_pid = A->pid

	// Verificar si otro proceso ya esta esperando a este proceso hijo
	if (child_process->waiting_pid != -1) {
		return -1;
	}

	// Si el proceso ya esta TERMINATED, reclamarlo inmediatamente
	if (child_process->state == TERMINATED) {
		// Marcar que este proceso (current) esta esperando al child
		child_process->waiting_pid = current->pid;
		return 0;
	}

	// Marcar que este proceso (current) esta esperando al child
	// Usar semantica correcta: child->waiting_pid = current->pid
	child_process->waiting_pid = current->pid;

	// Verificar nuevamente si el proceso termino despues de establecer waiting_pid
	// Esto evita race conditions donde el proceso termina entre la verificacion y el bloqueo
	child_process = get_process_by_pid(pid);
	if (child_process == NULL || child_process->state == TERMINATED) {
		if (child_process != NULL && child_process->state == TERMINATED) {
			// El proceso ya termino, mantener waiting_pid para que pueda ser reclamado
			// No hacer nada, waiting_pid ya esta establecido arriba
		}
		else {
			// El proceso no existe, limpiar waiting_pid
			child_process = get_process_by_pid(pid);
			if (child_process != NULL) {
				child_process->waiting_pid = -1;
			}
		}
		return (child_process != NULL) ? 0 : -1;
	}

	// Bloquear el proceso actual hasta que el hijo termine
	current->state = BLOCKED;
	removeFromScheduler(current);

	_force_scheduler_interrupt();

	return 0;
}

///////////////////////////////////////////////////////////
// int waitpid(process_id_t pid) {
//     if (pid <= 0)
//         return -1;

//     PCB *self = get_process_by_pid(get_current_pid());
//     if (self == NULL)
//         return -1;

//     PCB *child = get_process_by_pid(pid);
//     if (child == NULL)
//         return -1;

//     /* ¿Ya tiene un waiter? */
//     if (child->waiting_pid != -1)
//         return -1;

//     /* Si ya terminó, no bloquear */
//     if (child->state == TERMINATED) {
//         child->waiting_pid = self->pid;
//         return 0;
//     }

//     /* Registrar wait */
//     child->waiting_pid = self->pid;

//     /* Race condition: terminó justo ahora */
//     if (child->state == TERMINATED)
//         return 0;

//     /* Bloquear padre */
//     self->state = BLOCKED;
//     removeFromScheduler(self);

//     _force_scheduler_interrupt();

//     return 0;
// }
///////////////////////////////////////////////

int block_process(process_id_t pid) {
	if (pid == 0) {
		return -1;
	}

	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return -1;
	}

	// Si el proceso ya está TERMINATED, retornar 0 (éxito silencioso)
	// porque el efecto final es el mismo (el proceso no está corriendo)
	// Esto maneja race conditions donde el proceso fue terminado justo antes de bloquearlo
	if (process->state == TERMINATED) {
		return 0;
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
	if (process == NULL) {
		return -1;
	}

	// Si el proceso está TERMINATED, retornar 0 (éxito silencioso)
	// porque el efecto final es el mismo (el proceso no está corriendo)
	// Esto maneja race conditions donde el proceso fue terminado justo antes de desbloquearlo
	if (process->state == TERMINATED) {
		return 0;
	}

	// Si el proceso no está BLOCKED, retornar 0 (ya está desbloqueado o nunca estuvo bloqueado)
	if (process->state != BLOCKED) {
		return 0;
	}

	process->state = READY;
	process->waiting_ticks = 0;

	// CRITICO: Actualizar prioridad antes de agregar al scheduler
	// para que los cambios de prioridad mientras estaba bloqueado tomen efecto
	if (!process->foreground_priority_boost) {
		process->priority = clamp_priority(process->base_priority);
	}

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

	// Si el proceso está en READY, removerlo y agregarlo nuevamente
	// para que el scheduler lo reordene con la nueva prioridad
	if (process->state == READY && process->in_scheduler) {
		removeFromScheduler(process);
		addToScheduler(process);
	}

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
			// Copiar nombre de forma segura
			// Si el nombre esta vacio o es solo '?', usar un nombre por defecto basado en PID
			if (process_table[i].name[0] == '\0' ||
				(process_table[i].name[0] == '?' && process_table[i].name[1] == '\0')) {
				// Usar nombre por defecto basado en PID
				char default_name[16];
				default_name[0] = 'p';
				default_name[1] = 'r';
				default_name[2] = 'o';
				default_name[3] = 'c';
				default_name[4] = '_';
				// Convertir PID a string (simplificado)
				int pid = process_table[i].pid;
				int pos = 5;
				if (pid == 0) {
					default_name[pos++] = '0';
				}
				else {
					char pid_str[16];
					int pid_pos = 0;
					int temp_pid = pid;
					while (temp_pid > 0 && pid_pos < 15) {
						pid_str[pid_pos++] = '0' + (temp_pid % 10);
						temp_pid /= 10;
					}
					// Invertir
					for (int k = pid_pos - 1; k >= 0 && pos < 15; k--) {
						default_name[pos++] = pid_str[k];
					}
				}
				default_name[pos] = '\0';
				// Copiar nombre por defecto
				j = 0;
				while (j < MAX_PROCESS_NAME - 1 && default_name[j] != '\0') {
					buffer[count].name[j] = default_name[j];
					j++;
				}
			}
			else {
				// Copiar nombre normal
				while (j < MAX_PROCESS_NAME - 1) {
					buffer[count].name[j] = process_table[i].name[j];
					if (process_table[i].name[j] == '\0')
						break;
					j++;
				}
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
			// CRITICO: No liberar el stack del proceso actual
			// porque puede estar siendo usado durante el context switch
			if (&process_table[i] == current_process) {
				continue;
			}

			// NO liberar procesos que tienen un padre esperandolos (waiting_pid != -1)
			// Esto evita condiciones de carrera donde el padre se queda bloqueado
			if (process_table[i].waiting_pid != -1) {
				// Hay un proceso esperando a este proceso, no liberarlo todavia
				continue;
			}

			// CRITICO: Verificar que el proceso no este en el scheduler antes de liberar
			// Si esta en la ready queue, su stack puede ser usado en cualquier momento
			if (process_table[i].in_scheduler) {
				// Remover del scheduler antes de liberar recursos
				removeFromScheduler(&process_table[i]);
			}

			// CRITICO: Verificar que el proceso realmente este TERMINATED
			// y no haya cambiado de estado mientras se procesaba
			if (process_table[i].state == TERMINATED) {
				release_process_resources(&process_table[i]);
				// Marcar el slot como completamente libre
				process_table[i].state = TERMINATED;
			}
		}
	}
}
void *schedule(void *current_stack_pointer) {
	if (!initialized_flag) {
		return current_stack_pointer;
	}

	// Limpiar procesos terminados periodicamente
	cleanup_counter++;
	if (cleanup_counter >= CLEANUP_INTERVAL) {
		free_terminated_processes();
		cleanup_counter = 0;
	}
	// Limpieza adicional: si hay muchos procesos terminados, limpiar mas frecuentemente
	// Esto ayuda cuando hay muchas iteraciones del test
	else if (cleanup_counter >= 2) {
		int terminated_count = 0;
		for (int i = 1; i < MAX_PROCESSES; i++) {
			if (process_table[i].state == TERMINATED) {
				terminated_count++;
			}
		}
		// Si hay mas de 10 procesos terminados, forzar limpieza
		if (terminated_count > 10) {
			free_terminated_processes();
			cleanup_counter = 0;
		}
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

	// CRITICO: Validar que el proceso tenga todos los atributos necesarios antes de ejecutarlo
	// Esto previene RIP: 0 cuando entry_point es NULL
	if (next_process->state == TERMINATED || next_process->stack_base == NULL || next_process->stack_pointer == NULL ||
		(next_process->entry_point == NULL && next_process != idle_pcb)) {
		next_process = idle_pcb;
	}

	current_process = next_process;
	current_process->state = RUNNING;
	current_process->scheduler_counter++;
	current_process->waiting_ticks = 0;

	// Solo resetear prioridad si no hay foreground boost
	// Si hay boost, mantener la prioridad mínima
	if (!current_process->foreground_priority_boost) {
		current_process->priority = clamp_priority(current_process->base_priority);
	}

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
		// Si el entry_point es NULL, terminar el proceso inmediatamente
		// para evitar que se intente ejecutar codigo en direccion 0
		if (proc != NULL) {
			exit_current_process();
		}
		return;
	}

	((void (*)(int, char **)) proc->entry_point)(proc->argc, proc->argv);

	exit_current_process();
	__builtin_unreachable();
}
