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
		sys_read(fd, buffer, count);
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
		const char *name = va_arg(args, const char *);
		void (*entry_point)(int, char **) = va_arg(args, void (*)(int, char **));
		int argc = va_arg(args, int);
		char **argv = va_arg(args, char **);
		int priority_int = va_arg(args, int);
		uint8_t priority = (uint8_t) priority_int;
		process_id_t pid = sys_create_process(name, entry_point, argc, argv, priority);
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

	va_end(args);
	return 0; /* Retorno por defecto */
}

void sys_sleep(int secs) {
	sleep(secs);
}
void sys_read(FDS fd, char *buffer, size_t count) {
	if (fd == STDIN)
		read(buffer, count);
}
void sys_write(FDS fd, const char *buffer, size_t count, size_t color, size_t background) {
	if (fd == STDOUT)
		write(buffer, count, color, background);
	else if (fd == STDERR)
		write(buffer, count, 0x00ff0000, 0);
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

process_id_t sys_getpid() {
	return get_current_pid();
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
	process_id_t current_pid = get_current_pid();
	if (current_pid > 0) {
		kill_process(current_pid);
	}
}

int sys_waitpid(process_id_t pid) {
	if (pid <= 0) {
		return -1;
	}

	return waitpid(pid);
}