#include "include/clock.h"
#include "include/exceptions.h"
#include "include/interrupts.h"
#include "include/sound.h"
#include "include/stdinout.h"
#include "include/syscallDispatcher.h"
#include "include/time.h"
#include "include/videoDriver.h"
#include "memory-manager/include/memory_manager.h"
#include "scheduler/include/process.h"
#include <stdarg.h>
#include <stdint.h>

#include "include/pipe.h"

// Flag para habilitar logs de debug de pipes
#define PIPE_DEBUG 0 // Deshabilitado - cambiar a 1 para habilitar logs

// Flag para habilitar logs de debug de creacion de procesos
#define CREATE_PROC_DEBUG 0 // Deshabilitado - cambiar a 1 para habilitar logs

uint64_t syscallDispatcher(uint64_t rax, ...) {
	va_list args;
	va_start(args, rax);
	if (rax == 1) {
		FDS fd = va_arg(args, FDS);
		const char *buffer = va_arg(args, const char *);
		size_t count = va_arg(args, size_t);
		size_t color = va_arg(args, size_t);
		size_t background = va_arg(args, size_t);
		sys_write(fd, buffer, count, color, background);
	}
	else if (rax == 0) {
		FDS fd = va_arg(args, FDS);
		char *buffer = va_arg(args, char *);
		size_t count = va_arg(args, size_t);
		int result = sys_read(fd, buffer, count);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 84) {
		// sys_read_input - lee automaticamente de stdin sin requerir file descriptor
		char *buffer = va_arg(args, char *);
		size_t count = va_arg(args, size_t);
		int result = sys_read_input(buffer, count);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 85) {
		// sys_write_output - escribe automaticamente a stdout sin requerir file descriptor
		const char *buffer = va_arg(args, const char *);
		size_t count = va_arg(args, size_t);
		size_t color = va_arg(args, size_t);
		size_t background = va_arg(args, size_t);
#if PIPE_DEBUG
		vd_print("[SYSCALL] write_output count=");
		vd_print_dec(count);
		vd_print("\n");
#endif
		sys_write_output(buffer, count, color, background);
	}
	else if (rax == 35) {
		int secs = va_arg(args, int);
		sys_sleep(secs);
	}
	else if (rax == 28) {
		int zoom = va_arg(args, int);
		sys_zoom(zoom);
	}
	else if (rax == 43) {
		int color = va_arg(args, int);
		int x = va_arg(args, int);
		int y = va_arg(args, int);
		sys_draw(color, x, y);
	}
	else if (rax == 44) {
		int *width = va_arg(args, int *);
		int *height = va_arg(args, int *);
		sys_screenDetails(width, height);
	}
	else if (rax == 45) {
		int x = va_arg(args, int);
		int y = va_arg(args, int);
		sys_setCursor(x, y);
	}
	else if (rax == 46) {
		char *str = va_arg(args, char *);
		sys_getClock(str);
	}
	else if (rax == 47) {
		int index = va_arg(args, int);
		sys_playSound(index);
	}
	else if (rax == 48) {
		sys_clear_screen();
	}
	else if (rax == 50) {
		/* Syscall malloc */
		uint64_t size = va_arg(args, uint64_t);
		void *ptr = sys_malloc(size);
		va_end(args);
		return (uint64_t) ptr;
	}
	else if (rax == 51) {
		/* Syscall free */
		void *ptr = va_arg(args, void *);
		int result = sys_free(ptr);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 52) {
		/* Syscall mem_status */
		HeapState *state = va_arg(args, HeapState *);
		sys_mem_status(state);
	}
	else if (rax == 87) {
		uint64_t *milis = va_arg(args, uint64_t *);
		sys_getMilis(milis);
	}
	else if (rax == 2) {
		char *charac = va_arg(args, char *);
		sys_getcharNL(charac);
	}
	else if (rax == 12) {
		sys_impRegs();
	}
	/* Syscalls de procesos (Fase 3) */
	else if (rax == 60) {
		/* sys_create_process */
		// CRITICO: Todos los argumentos desde userland se pasan como uint64_t
		// Leer todos como uint64_t primero y luego hacer los casts apropiados
		uint64_t name_ptr = va_arg(args, uint64_t);
		const char *name = (const char *) name_ptr;
		uint64_t entry_point_ptr = va_arg(args, uint64_t);
		void (*entry_point)(int, char **) = (void (*)(int, char **)) entry_point_ptr;
		uint64_t argc_val = va_arg(args, uint64_t);
		int argc = (int) argc_val;
		uint64_t argv_ptr = va_arg(args, uint64_t);
		char **argv = (char **) argv_ptr;
		uint64_t priority_val = va_arg(args, uint64_t);
		uint8_t priority = (uint8_t) priority_val;

		// DEBUG: Verificar valores recibidos (siempre activo temporalmente)
#if CREATE_PROC_DEBUG
		vd_print("[SYSCALL] create_process name_ptr=0x");
		vd_print_hex(name_ptr);
		vd_print(" entry_point_ptr=0x");
		vd_print_hex(entry_point_ptr);
		vd_print(" argc=");
		vd_print_dec(argc_val);
		vd_print(" argv_ptr=0x");
		vd_print_hex(argv_ptr);
		vd_print(" priority=");
		vd_print_dec(priority_val);
		vd_print("\n");
#endif
		process_id_t pid = sys_create_process(name, entry_point, argc, argv, priority);
		va_end(args);
		return (uint64_t) pid;
	}
	else if (rax == 76) {
		/* sys_create_process_foreground */
		// CRITICO: Todos los argumentos desde userland se pasan como uint64_t
		// Leer todos como uint64_t primero y luego hacer los casts apropiados
		uint64_t name_ptr = va_arg(args, uint64_t);
		const char *name = (const char *) name_ptr;
		uint64_t entry_point_ptr = va_arg(args, uint64_t);
		void (*entry_point)(int, char **) = (void (*)(int, char **)) entry_point_ptr;
		uint64_t argc_val = va_arg(args, uint64_t);
		int argc = (int) argc_val;
		uint64_t argv_ptr = va_arg(args, uint64_t);
		char **argv = (char **) argv_ptr;
		uint64_t priority_val = va_arg(args, uint64_t);
		uint8_t priority = (uint8_t) priority_val;
		process_id_t pid = sys_create_process_foreground(name, entry_point, argc, argv, priority);
		va_end(args);
		return (uint64_t) pid;
	}
	else if (rax == 77) {
		/* sys_create_process_with_io */
		// CRITICO: Todos los argumentos desde userland se pasan como uint64_t
		// Leer todos como uint64_t primero y luego hacer los casts apropiados
		uint64_t name_ptr = va_arg(args, uint64_t);
		const char *name = (const char *) name_ptr;
		uint64_t entry_point_ptr = va_arg(args, uint64_t);
		void (*entry_point)(int, char **) = (void (*)(int, char **)) entry_point_ptr;
		uint64_t argc_val = va_arg(args, uint64_t);
		int argc = (int) argc_val;
		uint64_t argv_ptr = va_arg(args, uint64_t);
		char **argv = (char **) argv_ptr;
		uint64_t priority_val = va_arg(args, uint64_t);
		uint8_t priority = (uint8_t) priority_val;
		// CRITICO: Leer como uint64_t primero para ver el valor del puntero
		uint64_t user_config_ptr = va_arg(args, uint64_t);
		const process_io_config_t *user_config = (const process_io_config_t *) user_config_ptr;

#if PIPE_DEBUG
		vd_print("[SYSCALL] create_process_with_io name=");
		if (name != NULL) {
			vd_print(name);
		}
		else {
			vd_print("(null)");
		}
		vd_print(" user_config_ptr=0x");
		vd_print_hex(user_config_ptr);
		vd_print("\n");
#endif

		// CRITICO: Copiar config desde userland al kernel
		// Los punteros de userland no son validos en el kernel
		process_io_config_t kernel_config;
		const process_io_config_t *config = NULL;
		if (user_config != NULL && user_config_ptr != 0) {
			// Copiar la estructura desde userland
			// En un OS real, esto requeriria validacion de punteros y copia segura
			// Aqui asumimos que el puntero es valido y copiamos directamente
			kernel_config = *user_config;
			config = &kernel_config;
#if PIPE_DEBUG
			vd_print("[SYSCALL] create_process_with_io name=");
			if (name != NULL) {
				vd_print(name);
			}
			else {
				vd_print("(null)");
			}
			vd_print(" config copied stdin_type=");
			vd_print_dec(config->stdin_type);
			vd_print(" stdin_resource=");
			vd_print_dec(config->stdin_resource);
			vd_print(" stdout_type=");
			vd_print_dec(config->stdout_type);
			vd_print(" stdout_resource=");
			vd_print_dec(config->stdout_resource);
			vd_print("\n");
#endif
		}
		else {
#if PIPE_DEBUG
			vd_print("[SYSCALL] create_process_with_io name=");
			if (name != NULL) {
				vd_print(name);
			}
			else {
				vd_print("(null)");
			}
			vd_print(" config=(null) user_config_ptr=0x");
			vd_print_hex(user_config_ptr);
			vd_print("\n");
#endif
		}
		process_id_t pid = sys_create_process_with_io(name, entry_point, argc, argv, priority, config);
		va_end(args);
#if PIPE_DEBUG
		vd_print("[SYSCALL] create_process_with_io returned pid=");
		vd_print_dec(pid);
		vd_print("\n");
#endif
		return (uint64_t) pid;
	}
	else if (rax == 78) {
		/* sys_create_process_foreground_with_io */
		// CRITICO: Todos los argumentos desde userland se pasan como uint64_t
		// Leer todos como uint64_t primero y luego hacer los casts apropiados
		uint64_t name_ptr = va_arg(args, uint64_t);
		const char *name = (const char *) name_ptr;
		uint64_t entry_point_ptr = va_arg(args, uint64_t);
		void (*entry_point)(int, char **) = (void (*)(int, char **)) entry_point_ptr;
		uint64_t argc_val = va_arg(args, uint64_t);
		int argc = (int) argc_val;
		uint64_t argv_ptr = va_arg(args, uint64_t);
		char **argv = (char **) argv_ptr;
		uint64_t priority_val = va_arg(args, uint64_t);
		uint8_t priority = (uint8_t) priority_val;
		uint64_t user_config_ptr = va_arg(args, uint64_t);
		const process_io_config_t *user_config = (const process_io_config_t *) user_config_ptr;

		// CRITICO: Copiar config desde userland al kernel
		// Los punteros de userland no son validos en el kernel
		process_io_config_t kernel_config;
		const process_io_config_t *config = NULL;
		if (user_config != NULL) {
			// Copiar la estructura desde userland
			kernel_config = *user_config;
			config = &kernel_config;
#if PIPE_DEBUG
			vd_print("[SYSCALL] create_process_foreground_with_io name=");
			if (name != NULL) {
				vd_print(name);
			}
			else {
				vd_print("(null)");
			}
			vd_print(" config copied stdin_type=");
			vd_print_dec(config->stdin_type);
			vd_print(" stdout_type=");
			vd_print_dec(config->stdout_type);
			vd_print("\n");
#endif
		}
		process_id_t pid = sys_create_process_foreground_with_io(name, entry_point, argc, argv, priority, config);
		va_end(args);
		return (uint64_t) pid;
	}
	else if (rax == 61) {
		/* sys_getpid */
		process_id_t pid = sys_getpid();
		va_end(args);
		return (uint64_t) pid;
	}
	else if (rax == 62) {
		/* sys_kill */
		int pid_int = va_arg(args, int);
		process_id_t pid = (process_id_t) pid_int;
		int result = sys_kill(pid);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 63) {
		/* sys_block */
		int pid_int = va_arg(args, int);
		process_id_t pid = (process_id_t) pid_int;
		int result = sys_block(pid);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 64) {
		/* sys_unblock */
		int pid_int = va_arg(args, int);
		process_id_t pid = (process_id_t) pid_int;
		int result = sys_unblock(pid);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 65) {
		/* sys_nice */
		int pid_int = va_arg(args, int);
		process_id_t pid = (process_id_t) pid_int;
		int priority_int = va_arg(args, int);
		uint8_t new_priority = (uint8_t) priority_int;
		int result = sys_nice(pid, new_priority);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 66) {
		/* sys_yield */
		sys_yield();
	}
	else if (rax == 67) {
		/* sys_ps */
		ProcessInfo *buffer = va_arg(args, ProcessInfo *);
		int max_processes = va_arg(args, int);
		int count = sys_ps(buffer, max_processes);
		va_end(args);
		return (uint64_t) count;
	}
	else if (rax == 68) {
		/* sys_exit */
		sys_exit();
	}
	else if (rax == 69) {
		/* sys_waitpid */
		process_id_t pid = va_arg(args, process_id_t);
		int result = sys_waitpid(pid);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 70) {
		/* sys_sem_open */
		const char *name = va_arg(args, const char *);
		uint32_t initial_value = va_arg(args, uint32_t);
		int result = sys_sem_open(name, initial_value);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 71) {
		/* sys_sem_wait */
		const char *name = va_arg(args, const char *);
		int result = sys_sem_wait(name);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 72) {
		/* sys_sem_post */
		const char *name = va_arg(args, const char *);
		int result = sys_sem_post(name);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 73) {
		/* sys_sem_close */
		const char *name = va_arg(args, const char *);
		int result = sys_sem_close(name);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 74) {
		/* sys_set_foreground */
		process_id_t pid = va_arg(args, process_id_t);
		int result = sys_set_foreground(pid);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 75) {
		/* sys_clear_foreground */
		process_id_t pid = va_arg(args, process_id_t);
		int result = sys_clear_foreground(pid);
		va_end(args);
		return (uint64_t) result;
	}

	else if (rax == 100) {
		// sys_pipe_open
		char *name = va_arg(args, char *);
		int result = pipe_open(name);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 101) {
		// sys_pipe_close
		int id = va_arg(args, int);
		int result = pipe_close(id);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 102) {
		// sys_pipe_write
		int id = va_arg(args, int);
		const char *data = va_arg(args, const char *);
		uint64_t size = va_arg(args, uint64_t);
		int result = pipe_write(id, data, size);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 103) {
		// sys_pipe_read
		int id = va_arg(args, int);
		char *buffer = va_arg(args, char *);
		uint64_t size = va_arg(args, uint64_t);
		int result = pipe_read(id, buffer, size);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 79) {
		// sys_get_stdin_type
		uint32_t result = sys_get_stdin_type();
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 83) {
		// sys_get_stdout_type
		uint32_t result = sys_get_stdout_type();
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 80) {
		// sys_close_stdin
		int result = sys_close_stdin();
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 81) {
		// sys_close_stdin_pid
		int pid_int = va_arg(args, int);
		process_id_t pid = (process_id_t) pid_int;
		int result = sys_close_stdin_pid(pid);
		va_end(args);
		return (uint64_t) result;
	}
	else if (rax == 82) {
		// sys_get_foreground_process
		process_id_t result = sys_get_foreground_process();
		va_end(args);
		return (uint64_t) result;
	}

	va_end(args);
	return 0; /* Retorno por defecto */
}

void sys_sleep(int secs) {
	sleep(secs);
}
int sys_read(FDS fd, char *buffer, size_t count) {
	return process_read(get_current_pid(), fd, buffer, count);
}

int sys_read_input(char *buffer, size_t count) {
	// Lee automaticamente de stdin - los comandos no necesitan conocer el file descriptor
	return process_read(get_current_pid(), STDIN, buffer, count);
}

void sys_write(FDS fd, const char *buffer, size_t count, size_t color, size_t background) {
	process_write(get_current_pid(), fd, buffer, count, color, background);
}

void sys_write_output(const char *buffer, size_t count, size_t color, size_t background) {
	// Escribe automaticamente a stdout - los comandos no necesitan conocer el file descriptor
#if PIPE_DEBUG
	process_id_t current_pid = get_current_pid();
	vd_print("[SYS] sys_write_output pid=");
	vd_print_dec(current_pid);
	vd_print(" count=");
	vd_print_dec(count);
	vd_print("\n");
#endif
	process_write(get_current_pid(), STDOUT, buffer, count, color, background);
}
/* Syscall zoom - No soportado en modo texto VGA */
void sys_zoom(int zoom) {
	/* Funcionalidad solo disponible en modo grafico */
}

/* Syscall draw - No soportado en modo texto VGA */
void sys_draw(int color, int x, int y) {
	/* Funcionalidad solo disponible en modo grafico */
}

/* Syscall screenDetails - Retorna dimensiones de pantalla VGA texto */
void sys_screenDetails(int *width, int *height) {
	*width = 80;  /* 80 columnas */
	*height = 25; /* 25 filas */
}

/* Syscall setCursor - Posiciona el cursor en modo texto VGA */
void sys_setCursor(int x, int y) {
	vd_set_cursor(x, y);
}

/* Syscall clearScreen - Limpia la pantalla en modo texto VGA */
void sys_clear_screen() {
	vd_clear_screen();
}

void sys_getClock(char *str) {
	getClockTime(str);
}

void sys_playSound(int index) {
	playSoundSpeaker(index);
}

void sys_getMilis(uint64_t *milis) {
	_sti();
	*milis = getMiSe();
}

void sys_getcharNL(char *charac) {
	*charac = getcharNonLoop();
}

void sys_impRegs() {
	printRegistros(get_regs());
}

/* Syscall malloc - Reserva memoria dinamica */
void *sys_malloc(uint64_t size) {
	/* Validar parametro */
	if (size == 0 || size > MEMORY_SIZE) {
		return NULL;
	}

	return memory_alloc(memory_manager, size);
}

/* Syscall free - Libera memoria previamente reservada */
int sys_free(void *ptr) {
	/* Validar parametro */
	if (ptr == NULL) {
		return -1;
	}

	return memory_free(memory_manager, ptr);
}

/* Syscall mem_status - Obtiene informacion del estado de memoria */
void sys_mem_status(HeapState *state) {
	/* Validar parametro */
	if (state == NULL) {
		return;
	}

	memory_state_get(memory_manager, state);
}

/* Syscalls de procesos */

process_id_t sys_create_process(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
								uint8_t priority) {
	/* Validar parametros */
	if (entry_point == NULL) {
		return -1;
	}

	return create_process(name, entry_point, argc, argv, priority);
}

process_id_t sys_create_process_foreground(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
										   uint8_t priority) {
	if (entry_point == NULL) {
		return -1;
	}

	return create_process_foreground_with_io(name, entry_point, argc, argv, priority, NULL);
}

process_id_t sys_create_process_with_io(const char *name, void (*entry_point)(int, char **), int argc, char **argv,
										uint8_t priority, const process_io_config_t *config) {
	if (entry_point == NULL) {
		return -1;
	}
#if PIPE_DEBUG
	vd_print("[SYS] sys_create_process_with_io called name=");
	if (name != NULL) {
		vd_print(name);
	}
	else {
		vd_print("(null)");
	}
	vd_print("\n");
#endif
	process_id_t pid = create_process_with_io(name, entry_point, argc, argv, priority, config);
#if PIPE_DEBUG
	vd_print("[SYS] sys_create_process_with_io returned pid=");
	vd_print_dec(pid);
	vd_print("\n");
#endif
	return pid;
}

process_id_t sys_create_process_foreground_with_io(const char *name, void (*entry_point)(int, char **), int argc,
												   char **argv, uint8_t priority, const process_io_config_t *config) {
	if (entry_point == NULL) {
		return -1;
	}
	return create_process_foreground_with_io(name, entry_point, argc, argv, priority, config);
}

process_id_t sys_getpid() {
	return get_current_pid();
}

uint32_t sys_get_stdin_type() {
	PCB *process = get_process_by_pid(get_current_pid());
	if (process == NULL) {
		return PROCESS_IO_STDIN_KEYBOARD;
	}
	if (process->io_state.stdin_desc.type == IO_SOURCE_PIPE) {
		return PROCESS_IO_STDIN_PIPE;
	}
	return PROCESS_IO_STDIN_KEYBOARD;
}

uint32_t sys_get_stdout_type() {
	PCB *process = get_process_by_pid(get_current_pid());
	if (process == NULL) {
		return PROCESS_IO_STDOUT_SCREEN;
	}
	if (process->io_state.stdout_desc.type == IO_SINK_PIPE) {
		return PROCESS_IO_STDOUT_PIPE;
	}
	return PROCESS_IO_STDOUT_SCREEN;
}

int sys_close_stdin(void) {
	PCB *process = get_process_by_pid(get_current_pid());
	if (process == NULL) {
		return -1;
	}
	process->io_state.stdin_eof = 1;
	return 0;
}

int sys_close_stdin_pid(process_id_t pid) {
	PCB *process = get_process_by_pid(pid);
	if (process == NULL) {
		return -1;
	}
	process->io_state.stdin_eof = 1;
	return 0;
}

process_id_t sys_get_foreground_process(void) {
	return get_foreground_process();
}

int sys_kill(process_id_t pid) {
	/* Validar parametro */
	if (pid <= 0) {
		return -1;
	}

	return kill_process(pid);
}

int sys_block(process_id_t pid) {
	/* Validar parametro */
	if (pid <= 0) {
		return -1;
	}

	return block_process(pid);
}

int sys_unblock(process_id_t pid) {
	/* Validar parametro */
	if (pid <= 0) {
		return -1;
	}

	return unblock_process(pid);
}

int sys_nice(process_id_t pid, uint8_t new_priority) {
	/* Validar parametros */
	if (pid <= 0 || new_priority > MAX_PRIORITY) {
		return -1;
	}

	return change_priority(pid, new_priority);
}

void sys_yield() {
	/* Cede voluntariamente el CPU */
	extern void force_context_switch();
	force_context_switch();
}

int sys_ps(ProcessInfo *buffer, int max_processes) {
	/* Validar parametros */
	if (buffer == NULL || max_processes <= 0) {
		return -1;
	}

	return get_processes_info(buffer, max_processes);
}

void sys_exit() {
	exit_current_process();
}

int sys_waitpid(process_id_t pid) {
	if (pid <= 0) {
		return -1;
	}

	return waitpid(pid);
}

int sys_sem_open(const char *name, uint32_t initial_value) {
	if (name == NULL) {
		return -1;
	}
	return sem_open(name, initial_value);
}

int sys_sem_wait(const char *name) {
	if (name == NULL) {
		return -1;
	}
	return sem_wait(name);
}

int sys_sem_post(const char *name) {
	if (name == NULL) {
		return -1;
	}
	return sem_post(name);
}

int sys_sem_close(const char *name) {
	if (name == NULL) {
		return -1;
	}
	return sem_close(name);
}

int sys_set_foreground(process_id_t pid) {
	return set_foreground_process(pid);
}

int sys_clear_foreground(process_id_t pid) {
	return clear_foreground_process(pid);
}