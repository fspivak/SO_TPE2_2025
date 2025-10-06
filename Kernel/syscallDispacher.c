#include "include/clock.h"
#include "include/exceptions.h"
#include "include/interrupts.h"
#include "include/sound.h"
#include "include/stdinout.h"
#include "include/syscallDispatcher.h"
#include "include/time.h"
#include "include/videoDriver.h"
#include "memory-manager/include/memory_manager.h"
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