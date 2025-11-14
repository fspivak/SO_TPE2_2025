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

static void release_process_resources(PCB *process) {
	if (process == NULL) {
		return;
	}

	removeFromScheduler(process);

	release_process_arguments(process);

	if (process->stack_base != NULL) {
		// Verificar que memory_manager sea valido antes de usarlo
		if (memory_manager != NULL) {
			// CRITICO: memory_free requiere el puntero original de memory_alloc
			// No el puntero alineado. El puntero original se guarda justo antes
			// del stack_base alineado cuando se crea el proceso
			uint64_t stack_base_addr = (uint64_t) process->stack_base;
			uint64_t ptr_location = stack_base_addr - sizeof(void *);

			// Validar que la ubicacion del puntero sea segura
			// Debe estar alineada, dentro del rango de memoria valida, y antes del stack_base
			if (ptr_location >= 0x600000 && ptr_location < 0x800000 && ptr_location < stack_base_addr &&
				(ptr_location % sizeof(void *)) == 0) {
				// Leer el puntero original de forma segura
				void *original_ptr = *((void **) ptr_location);

				// Validar que el puntero original sea valido
				// Debe estar dentro del rango de memoria y antes o igual al stack_base
				if (original_ptr != NULL && (uint64_t) original_ptr >= 0x600000 && (uint64_t) original_ptr < 0x800000 &&
					(uint64_t) original_ptr <= stack_base_addr) {
					// Liberar usando el puntero original (el unico valido para memory_free)
					memory_free(memory_manager, original_ptr);
				}
				// Si el puntero original no es valido, no liberar para evitar corrupcion
				// Esto puede indicar corrupcion del stack o proceso antiguo sin el puntero guardado
			}
			// Si no se puede leer el puntero original de forma segura, no liberar
			// para evitar corrupcion de memoria o double free
		}
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

static process_id_t create_process_internal(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
											uint8_t priority, const process_io_config_t *io_config,
											int assign_foreground) {
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
	// Limpiar recursos si existen antes de reutilizar el slot
	if (new_process->stack_base != NULL || new_process->argv != NULL) {
		release_process_resources(new_process);
	}

	// CRITICO: Inicializar TODOS los atributos críticos explícitamente
	// para evitar valores residuales que puedan causar crashes
	// Especialmente entry_point, argc, argv, stack_base, stack_pointer
	// que deben estar en un estado válido antes de establecer state = READY
	new_process->entry_point = entry_point; // Establecer INMEDIATAMENTE para evitar RIP: 0
	new_process->argc = 0;					// Se establecerá en duplicate_arguments()
	new_process->argv = NULL;				// Se establecerá en duplicate_arguments()
	new_process->stack_base = NULL;			// Se asignará después
	new_process->stack_pointer = NULL;		// Se asignará después
	new_process->in_scheduler = 0;			// Se establecerá en addToScheduler()
	new_process->waiting_pid = -1;			// Ningún proceso está esperando a este nuevo proceso

	new_process->pid = get_next_valid_pid();
	new_process->state = READY; // Solo establecer READY después de entry_point válido
	new_process->priority = priority;
	new_process->base_priority = priority;
	new_process->saved_priority = priority;
	new_process->foreground_priority_boost = 0;
	new_process->scheduler_counter = 0;
	new_process->waiting_ticks = 0;
	new_process->pending_cleanup = 0;
	new_process->parent_pid = get_current_pid();
	new_process->has_foreground = 0;
	init_process_io_defaults(&new_process->io_state);
	// Copiar nombre de forma segura desde userland
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

	PCB *parent_process = get_process_by_pid(new_process->parent_pid);
	if (parent_process != NULL) {
		new_process->io_state = parent_process->io_state;
		new_process->io_state.stdin_eof = 0;
		// NO llamar retain_process_io_resources aqui si hay io_config
		// porque override_process_io cambiara el estado y necesitamos registrar
		// con el estado final, no con el heredado
		if (io_config == NULL) {
			// Solo registrar recursos si NO vamos a hacer override
			if (retain_process_io_resources(new_process) != 0) {
				init_process_io_defaults(&new_process->io_state);
				new_process->state = TERMINATED;
				return -1;
			}
		}
	}
	else {
		init_process_io_defaults(&new_process->io_state);
	}

	if (io_config != NULL) {
		// Guardar estado antes de override para detectar qué cambió
		ProcessIOState state_before = new_process->io_state;
#if PIPE_DEBUG
		vd_print("[PROC] Creating process with io_config pid=");
		vd_print_dec(new_process->pid);
		vd_print(" name=");
		vd_print(new_process->name);
		vd_print(" stdin_type=");
		vd_print_dec(io_config->stdin_type);
		vd_print(" stdin_resource=");
		vd_print_dec(io_config->stdin_resource);
		vd_print(" stdout_type=");
		vd_print_dec(io_config->stdout_type);
		vd_print(" stdout_resource=");
		vd_print_dec(io_config->stdout_resource);
		vd_print("\n");
#endif
		if (override_process_io(new_process, io_config) != 0) {
#if PIPE_DEBUG
			vd_print("[PROC] ERROR: override_process_io failed for pid=");
			vd_print_dec(new_process->pid);
			vd_print("\n");
#endif
			// Si hay error, liberar recursos que pudieron haberse registrado antes
			release_process_io_resources(new_process);
			new_process->state = TERMINATED;
			return -1;
		}

		// CRITICO: override_process_io registra los recursos cuando cambia a PIPE
		// Pero si se usa INHERIT, mantiene lo heredado sin registrar
		// Necesitamos registrar los recursos heredados que son pipes y no fueron cambiados
		// Solo registrar si el descriptor no cambió (era PIPE antes y sigue siendo PIPE)
		if (state_before.stdin_desc.type == IO_SOURCE_PIPE && new_process->io_state.stdin_desc.type == IO_SOURCE_PIPE &&
			state_before.stdin_desc.resource_id == new_process->io_state.stdin_desc.resource_id &&
			new_process->io_state.stdin_desc.resource_id >= 0) {
			if (pipe_register_reader(new_process->io_state.stdin_desc.resource_id) != 0) {
				release_process_io_resources(new_process);
				new_process->state = TERMINATED;
				return -1;
			}
		}

		if (state_before.stdout_desc.type == IO_SINK_PIPE && new_process->io_state.stdout_desc.type == IO_SINK_PIPE &&
			state_before.stdout_desc.resource_id == new_process->io_state.stdout_desc.resource_id &&
			new_process->io_state.stdout_desc.resource_id >= 0) {
			if (pipe_register_writer(new_process->io_state.stdout_desc.resource_id) != 0) {
				if (state_before.stdin_desc.type == IO_SOURCE_PIPE &&
					new_process->io_state.stdin_desc.type == IO_SOURCE_PIPE &&
					state_before.stdin_desc.resource_id == new_process->io_state.stdin_desc.resource_id) {
					pipe_unregister_reader(new_process->io_state.stdin_desc.resource_id);
				}
				release_process_io_resources(new_process);
				new_process->state = TERMINATED;
				return -1;
			}
		}

		if (state_before.stderr_desc.type == IO_SINK_PIPE && new_process->io_state.stderr_desc.type == IO_SINK_PIPE &&
			state_before.stderr_desc.resource_id == new_process->io_state.stderr_desc.resource_id &&
			new_process->io_state.stderr_desc.resource_id >= 0) {
			if (pipe_register_writer(new_process->io_state.stderr_desc.resource_id) != 0) {
				if (state_before.stdout_desc.type == IO_SINK_PIPE &&
					new_process->io_state.stdout_desc.type == IO_SINK_PIPE &&
					state_before.stdout_desc.resource_id == new_process->io_state.stdout_desc.resource_id) {
					pipe_unregister_writer(new_process->io_state.stdout_desc.resource_id);
				}
				if (state_before.stdin_desc.type == IO_SOURCE_PIPE &&
					new_process->io_state.stdin_desc.type == IO_SOURCE_PIPE &&
					state_before.stdin_desc.resource_id == new_process->io_state.stdin_desc.resource_id) {
					pipe_unregister_reader(new_process->io_state.stdin_desc.resource_id);
				}
				release_process_io_resources(new_process);
				new_process->state = TERMINATED;
				return -1;
			}
		}
	}
	else if (parent_process != NULL) {
		// Si no hay io_config pero hay parent, los recursos ya se registraron arriba
		// (o no había nada que registrar si el parent no usaba pipes)
	}

	if (duplicate_arguments(new_process, argc, argv) < 0) {
		release_process_io_resources(new_process);
		new_process->state = TERMINATED;
		return -1;
	}

	// Forzar limpieza agresiva antes de asignar stack para evitar memory leaks
	free_terminated_processes();

	// Asignar stack con espacio extra para alineacion y puntero original
	// Necesitamos STACK_SIZE + 16 bytes para alineacion + 8 bytes para guardar puntero original
	void *stack_allocated = memory_alloc(memory_manager, STACK_SIZE + 16 + sizeof(void *));
	if (stack_allocated == NULL) {
		free_terminated_processes();
		stack_allocated = memory_alloc(memory_manager, STACK_SIZE + 16 + sizeof(void *));
		if (stack_allocated == NULL) {
			release_process_arguments(new_process);
			release_process_io_resources(new_process);
			new_process->state = TERMINATED;
			return -1;
		}
	}
	// Alinear stack_base a 16 bytes desde el principio, dejando espacio para el puntero
	uint64_t stack_base_aligned = ((uint64_t) stack_allocated + sizeof(void *) + 15) & ~((uint64_t) 0xF);
	// Guardar el puntero original justo antes del stack_base alineado
	*((void **) (stack_base_aligned - sizeof(void *))) = stack_allocated;
	new_process->stack_base = (void *) stack_base_aligned;

	uint64_t stack_limit = stack_base_aligned + STACK_SIZE;
	uint64_t user_rsp = (stack_limit - USER_STACK_GUARD_SIZE) & ~((uint64_t) 0xF);

	// Verificar que el stack este dentro del area asignada
	// stack_base_aligned debe estar dentro de stack_allocated...stack_allocated_end
	// user_rsp debe estar dentro de stack_base_aligned...stack_limit
	uint64_t stack_allocated_end = (uint64_t) stack_allocated + STACK_SIZE + 16 + sizeof(void *);
	if (stack_base_aligned < (uint64_t) stack_allocated || stack_base_aligned >= stack_allocated_end ||
		stack_limit > stack_allocated_end || user_rsp < stack_base_aligned || user_rsp >= stack_limit) {
		memory_free(memory_manager, stack_allocated);
		release_process_arguments(new_process);
		release_process_io_resources(new_process);
		new_process->state = TERMINATED;
		return -1;
	}

	void *stack_top = (void *) user_rsp;
	new_process->stack_pointer =
		set_process_stack(new_process->argc, new_process->argv, stack_top, (void *) new_process);

	// DEBUG CRITICO: Solo imprimir si hay un problema
	if (new_process->stack_pointer == NULL) {
		vd_print("[CREATE_PROC] ERROR: set_process_stack returned NULL\n");
		release_process_resources(new_process);
		new_process->state = TERMINATED;
		return -1;
	}

	if (addToScheduler(new_process) < 0) {
		release_process_resources(new_process);
		new_process->state = TERMINATED;
		return -1;
	}

	if (assign_foreground) {
		if (set_foreground_process(new_process->pid) < 0) {
			terminate_process(new_process, 1);
			return -1;
		}
	}

	return new_process->pid;
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

	if (foregroundProcessPid == pid) {
		release_foreground_owner();
	}

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
