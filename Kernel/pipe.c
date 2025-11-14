
#include "include/pipe.h"
#include "include/stringKernel.h"
#include "include/videoDriver.h"
#include "scheduler/include/process.h"
#include "scheduler/include/semaphore.h"
// #include "include/memoryManager.h"

// Flag para habilitar logs de debug de pipes
#define PIPE_DEBUG 0 // Deshabilitado - cambiar a 1 para habilitar logs

static Pipe pipes[MAX_PIPES];

static void int_to_string(int value, char *buffer, int buffer_size) {
	if (buffer == NULL || buffer_size <= 0) {
		return;
	}

	if (value == 0) {
		if (buffer_size > 1) {
			buffer[0] = '0';
			buffer[1] = '\0';
		}
		return;
	}

	int idx = 0;
	int v = value;
	char temp[16];
	while (v > 0 && idx < (int) sizeof(temp)) {
		temp[idx++] = (char) ('0' + (v % 10));
		v /= 10;
	}

	int out = 0;
	while (idx > 0 && out < buffer_size - 1) {
		buffer[out++] = temp[--idx];
	}
	buffer[out] = '\0';
}

static void build_pipe_sem_name(int index, const char *suffix, char *target, int target_size) {
	if (target == NULL || target_size <= 0 || suffix == NULL) {
		return;
	}

	const char prefix[] = "pipe_";
	int pos = 0;
	for (int i = 0; prefix[i] != '\0' && pos < target_size - 1; i++) {
		target[pos++] = prefix[i];
	}

	char number_buffer[16];
	int_to_string(index, number_buffer, sizeof(number_buffer));
	for (int i = 0; number_buffer[i] != '\0' && pos < target_size - 1; i++) {
		target[pos++] = number_buffer[i];
	}

	for (int i = 0; suffix[i] != '\0' && pos < target_size - 1; i++) {
		target[pos++] = suffix[i];
	}

	target[pos] = '\0';
}

static void pipe_reset(Pipe *p) {
	if (p == NULL) {
		return;
	}

	p->name[0] = '\0';
	p->readIndex = 0;
	p->writeIndex = 0;
	p->count = 0;
	p->readers = 0;
	p->writers = 0;
	p->sem_mutex_name[0] = '\0';
	p->sem_read_name[0] = '\0';
	p->sem_write_name[0] = '\0';
	p->sem_mutex_id = -1;
	p->sem_read_id = -1;
	p->sem_write_id = -1;
	p->active = 0;
}

void init_pipes() {
	for (int i = 0; i < MAX_PIPES; i++) {
		pipe_reset(&pipes[i]);
	}
}

static int find_pipe(char *name) {
	for (int i = 0; i < MAX_PIPES; i++)
		if (pipes[i].active && kstrcmp(pipes[i].name, name) == 0)
			return i;
	return -1;
}

static void pipe_destroy_if_unused(Pipe *p) {
	if (p == NULL) {
		return;
	}

	if (p->active && p->readers == 0 && p->writers == 0) {
		if (p->sem_mutex_name[0] != '\0') {
			sem_close(p->sem_mutex_name);
		}
		if (p->sem_read_name[0] != '\0') {
			sem_close(p->sem_read_name);
		}
		if (p->sem_write_name[0] != '\0') {
			sem_close(p->sem_write_name);
		}
		pipe_reset(p);
	}
}

int pipe_open(char *name) {
	int idx = find_pipe(name);
	if (idx >= 0) {
		return idx;
	}

	// Buscar un slot libre
	for (int i = 0; i < MAX_PIPES; i++) {
		if (!pipes[i].active) {
			Pipe *p = &pipes[i];
			pipe_reset(p);
			p->active = 1;
			if (name != NULL) {
				kstrncpy(p->name, name, sizeof(p->name));
			}

			build_pipe_sem_name(i, "_mutex", p->sem_mutex_name, sizeof(p->sem_mutex_name));
			build_pipe_sem_name(i, "_read", p->sem_read_name, sizeof(p->sem_read_name));
			build_pipe_sem_name(i, "_write", p->sem_write_name, sizeof(p->sem_write_name));

			if (sem_open(p->sem_mutex_name, 1) < 0 || sem_open(p->sem_read_name, 0) < 0 ||
				sem_open(p->sem_write_name, PIPE_BUFFER_SIZE) < 0) {
				pipe_destroy_if_unused(p);
				return -1;
			}
#if PIPE_DEBUG
			vd_print("[PIPE] Created pipe id=");
			vd_print_dec(i);
			vd_print(" name=");
			if (name != NULL) {
				vd_print(name);
			}
			else {
				vd_print("(null)");
			}
			vd_print(" readers=0 writers=0\n");
#endif
			return i;
		}
	}
	return -1;
}

int pipe_close(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];
	pipe_destroy_if_unused(p);
	return 0;
}

int pipe_write(int id, const char *data, uint64_t size) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active || data == NULL) {
#if PIPE_DEBUG
		vd_print("[PIPE] ERROR: write failed - invalid pipe id=");
		vd_print_dec(id);
		vd_print("\n");
#endif
		return -1;
	}

	if (size == 0)
		return 0;

	Pipe *p = &pipes[id];
	uint64_t written = 0;
#if PIPE_DEBUG
	vd_print("[PIPE] WRITE attempt pipe id=");
	vd_print_dec(id);
	vd_print(" size=");
	vd_print_dec(size);
	vd_print(" readers=");
	vd_print_dec(p->readers);
	vd_print(" writers=");
	vd_print_dec(p->writers);
	vd_print(" count=");
	vd_print_dec(p->count);
	vd_print("\n");
#endif

	// Logica simplificada como en Nahue: sem_wait -> escribir -> sem_post
	// Escribir caracter por caracter para permitir escritura en tiempo real
	for (uint64_t i = 0; i < size; i++) {
		// Verificar que el pipe sigue activo
		if (!p->active) {
			return (int) written;
		}

		// Esperar espacio disponible - se bloquea si el buffer esta lleno
		if (sem_wait(p->sem_write_name) != 0) {
			// Error o pipe cerrado
			return (int) written;
		}

		// Obtener mutex para acceso exclusivo al buffer
		if (sem_wait(p->sem_mutex_name) != 0) {
			sem_post(p->sem_write_name);
			return (int) written;
		}

		// Verificar nuevamente que el pipe sigue activo
		if (!p->active) {
			sem_post(p->sem_mutex_name);
			sem_post(p->sem_write_name);
			return (int) written;
		}

		// Escribir caracter al buffer circular
		// El buffer circular permite que el escritor llene mientras el lector consume
		// writeIndex se mueve circularmente usando modulo PIPE_BUFFER_SIZE
		p->buffer[p->writeIndex] = data[i];
		p->writeIndex = (p->writeIndex + 1) % PIPE_BUFFER_SIZE;
		p->count++;
		written++;

		// Liberar mutex
		sem_post(p->sem_mutex_name);

		// Despertar al lector cuando hay datos disponibles en el buffer circular
		// (como en Nahue: sem_post(read_sem))
		// El lector se despertara y leera del buffer circular
		sem_post(p->sem_read_name);
#if PIPE_DEBUG
		vd_print("[PIPE] WRITE wrote char='");
		if (data[i] >= 32 && data[i] <= 126) {
			char c = data[i];
			vd_print(&c);
		}
		else {
			vd_print("?");
		}
		vd_print("' pipe id=");
		vd_print_dec(id);
		vd_print(" count=");
		vd_print_dec(p->count);
		vd_print("\n");
#endif
	}
#if PIPE_DEBUG
	vd_print("[PIPE] WRITE completed pipe id=");
	vd_print_dec(id);
	vd_print(" bytes_written=");
	vd_print_dec(written);
	vd_print("\n");
#endif
	return (int) written;
}

int pipe_read(int id, char *buffer, uint64_t size) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active || buffer == NULL) {
#if PIPE_DEBUG
		vd_print("[PIPE] ERROR: read failed - invalid pipe id=");
		vd_print_dec(id);
		vd_print("\n");
#endif
		return -1;
	}

	if (size == 0)
		return 0;

	Pipe *p = &pipes[id];
	uint64_t read_count = 0;
#if PIPE_DEBUG
	vd_print("[PIPE] READ attempt pipe id=");
	vd_print_dec(id);
	vd_print(" size=");
	vd_print_dec(size);
	vd_print(" readers=");
	vd_print_dec(p->readers);
	vd_print(" writers=");
	vd_print_dec(p->writers);
	vd_print(" count=");
	vd_print_dec(p->count);
	vd_print("\n");
#endif

	// Logica simplificada como en Nahue: sem_wait -> leer -> sem_post
	// El lector se bloquea automaticamente si no hay datos (sem_read_name == 0)
	for (uint64_t i = 0; i < size; i++) {
		// Verificar que el pipe sigue activo
		if (!p->active) {
			return (int) read_count;
		}

		// Esperar datos disponibles - se bloquea si no hay datos
		// Cuando el escritor escribe, hace sem_post y despierta al lector
		// CRITICO: sem_wait deberia bloquearse si no hay datos (value == 0)
		// Si retorna -1, es un error critico (semaforo no existe, cola llena, etc.)
		// NO debemos retornar EOF aqui - sem_wait debe bloquearse hasta que haya datos
#if PIPE_DEBUG
		vd_print("[PIPE] READ blocking on sem_wait pipe id=");
		vd_print_dec(id);
		vd_print("\n");
#endif
		int wait_result = sem_wait(p->sem_read_name);
#if PIPE_DEBUG
		vd_print("[PIPE] READ woke up from sem_wait pipe id=");
		vd_print_dec(id);
		vd_print(" result=");
		vd_print_dec(wait_result);
		vd_print("\n");
#endif
		if (wait_result != 0) {
			// Error critico: semaforo no existe o cola llena
			// Si ya leimos algo, retornar lo leido
			if (read_count > 0) {
				return (int) read_count;
			}
			// Si el pipe esta cerrado, retornar EOF
			if (!p->active) {
				return 0; // EOF
			}
			// Error critico - el semaforo deberia existir y bloquearse
			// Esto indica un problema de configuracion
			return -1; // Error
		}

		// Verificar nuevamente que el pipe sigue activo
		if (!p->active) {
			sem_post(p->sem_read_name);
			return (int) read_count;
		}

		// Obtener mutex para acceso exclusivo al buffer
		if (sem_wait(p->sem_mutex_name) != 0) {
			sem_post(p->sem_read_name);
			if (read_count > 0) {
				return (int) read_count;
			}
			return -1;
		}

		// Si sem_wait se despertó, deberia haber datos
		// Si no hay datos, puede ser porque:
		// 1. Otro lector los tomó (race condition) - continuar esperando
		// 2. El escritor terminó y nos despertó para indicar EOF - retornar EOF
		// Verificamos writers SOLO cuando no hay datos para detectar EOF
		if (p->count == 0) {
			// No hay datos - verificar si es EOF o race condition
			if (p->writers == 0) {
				// No hay escritores y no hay datos - EOF
				// El escritor terminó y nos despertó para indicar EOF
				// CRITICO: Liberar ambos semaforos antes de retornar EOF
				// sem_read_name fue adquirido con sem_wait, debe liberarse
				sem_post(p->sem_mutex_name);
				sem_post(p->sem_read_name);
#if PIPE_DEBUG
				vd_print("[PIPE] READ EOF detected pipe id=");
				vd_print_dec(id);
				vd_print(" bytes_read=");
				vd_print_dec(read_count);
				vd_print("\n");
#endif
				// Retornar lo que hayamos leido hasta ahora (puede ser 0 si no leimos nada)
				return (int) read_count;
			}
			// Hay escritores pero no hay datos - race condition con otro lector
			// Liberar mutex y continuar esperando
			sem_post(p->sem_mutex_name);
			sem_post(p->sem_read_name);
			i--; // No incrementar i, intentar de nuevo
			continue;
		}

		// Leer caracter del buffer circular
		// El buffer circular permite que el escritor llene mientras el lector consume
		// readIndex y writeIndex se mueven circularmente usando modulo PIPE_BUFFER_SIZE
		buffer[i] = p->buffer[p->readIndex];
		p->readIndex = (p->readIndex + 1) % PIPE_BUFFER_SIZE;
		p->count--;
		read_count++;

		// Liberar mutex
		sem_post(p->sem_mutex_name);

		// Liberar espacio en el buffer circular (como en Nahue: sem_post(write_sem))
		// Esto permite que el escritor continue escribiendo si estaba bloqueado
		sem_post(p->sem_write_name);
	}
#if PIPE_DEBUG
	vd_print("[PIPE] READ completed pipe id=");
	vd_print_dec(id);
	vd_print(" bytes_read=");
	vd_print_dec(read_count);
	vd_print("\n");
#endif
	return (int) read_count;
}

int pipe_register_reader(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active) {
#if PIPE_DEBUG
		vd_print("[PIPE] ERROR: register_reader failed - invalid pipe id=");
		vd_print_dec(id);
		vd_print("\n");
#endif
		return -1;
	}
	pipes[id].readers++;
#if PIPE_DEBUG
	vd_print("[PIPE] Registered READER on pipe id=");
	vd_print_dec(id);
	vd_print(" readers=");
	vd_print_dec(pipes[id].readers);
	vd_print(" writers=");
	vd_print_dec(pipes[id].writers);
	vd_print("\n");
#endif
	return 0;
}

int pipe_unregister_reader(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active) {
		return -1;
	}
	if (pipes[id].readers > 0) {
		pipes[id].readers--;
	}
	pipe_destroy_if_unused(&pipes[id]);
	return 0;
}

int pipe_register_writer(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active) {
#if PIPE_DEBUG
		vd_print("[PIPE] ERROR: register_writer failed - invalid pipe id=");
		vd_print_dec(id);
		vd_print("\n");
#endif
		return -1;
	}
	pipes[id].writers++;
#if PIPE_DEBUG
	vd_print("[PIPE] Registered WRITER on pipe id=");
	vd_print_dec(id);
	vd_print(" readers=");
	vd_print_dec(pipes[id].readers);
	vd_print(" writers=");
	vd_print_dec(pipes[id].writers);
	vd_print("\n");
#endif
	return 0;
}

int pipe_unregister_writer(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active) {
		return -1;
	}

	// Guardar el numero de escritores antes de decrementar
	int had_writers = (pipes[id].writers > 0);

	if (pipes[id].writers > 0) {
		pipes[id].writers--;
	}

#if PIPE_DEBUG
	vd_print("[PIPE] Unregister WRITER pipe id=");
	vd_print_dec(id);
	vd_print(" had_writers=");
	vd_print_dec(had_writers);
	vd_print(" writers=");
	vd_print_dec(pipes[id].writers);
	vd_print(" readers=");
	vd_print_dec(pipes[id].readers);
	vd_print(" count=");
	vd_print_dec(pipes[id].count);
	vd_print("\n");
#endif

	// CRITICO: Despertar lectores cuando no hay mas writers
	// Si habia writers y ahora no hay, o si nunca hubo writers pero hay lectores esperando,
	// despertar a los lectores para que puedan detectar EOF
	// Esto maneja el caso donde wc termina sin escribir (debe retornar EOF inmediatamente)
	if (pipes[id].writers == 0 && pipes[id].readers > 0) {
		// No hay mas writers - despertar a todos los lectores bloqueados para que puedan detectar EOF
		// Hacer sem_post en sem_read_name para cada lector bloqueado
		// Nota: esto puede despertar mas lectores de los necesarios, pero es seguro
		// porque cada lector verificara si hay datos o si debe retornar EOF
#if PIPE_DEBUG
		vd_print("[PIPE] Waking up readers (EOF) pipe id=");
		vd_print_dec(id);
		vd_print(" readers=");
		vd_print_dec(pipes[id].readers);
		vd_print(" had_writers=");
		vd_print_dec(had_writers);
		vd_print(" count=");
		vd_print_dec(pipes[id].count);
		vd_print("\n");
#endif
		for (int i = 0; i < pipes[id].readers; i++) {
			sem_post(pipes[id].sem_read_name);
		}
	}

	pipe_destroy_if_unused(&pipes[id]);
	return 0;
}

int pipe_has_writers(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active) {
		return 0;
	}
	return pipes[id].writers > 0 ? 1 : 0;
}
