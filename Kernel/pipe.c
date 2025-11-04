//////////////TODO: Borrar esta version sin semaforos//////////////7
// #include "include/pipe.h"
// #include "include/lib.h"
// #include "include/stringKernel.h"
// #include "scheduler/include/process.h"
// // Tabla global de pipes
// static Pipe pipes[MAX_PIPES];

// // Inicializa la tabla de pipes
// void init_pipes() {
// 	for (int i = 0; i < MAX_PIPES; i++)
// 		pipes[i].active = 0;
// }

// // Busca un pipe existente por nombre
// static int find_pipe(char *name) {
// 	for (int i = 0; i < MAX_PIPES; i++) {
// 		if (pipes[i].active && !kstrcmp(pipes[i].name, name))
// 			return i;
// 	}
// 	return -1;
// }

// // Crea o abre un pipe
// int pipe_open(char *name) {
// 	int index = find_pipe(name);
// 	if (index >= 0)
// 		return index;

// 	for (int i = 0; i < MAX_PIPES; i++) {
// 		if (!pipes[i].active) {
// 			Pipe *p = &pipes[i];
// 			p->active = 1;
// 			kstrncpy(p->name, name, sizeof(p->name));
// 			p->readIndex = p->writeIndex = p->count = 0;
// 			p->readers = p->writers = 1;
// 			return i;
// 		}
// 	}
// 	return -1; // No hay lugar
// }

// // Cierra un pipe
// int pipe_close(int id) {
// 	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
// 		return -1;

// 	Pipe *p = &pipes[id];
// 	if (--p->readers == 0 && --p->writers == 0)
// 		p->active = 0;

// 	return 0;
// }

// // Escribe datos en el pipe
// int pipe_write(int id, const char *data, uint64_t size) {
// 	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
// 		return -1;

// 	Pipe *p = &pipes[id];

// 	for (uint64_t i = 0; i < size; i++) {
// 		// Esperar si el buffer está lleno
// 		while (p->count == PIPE_BUFFER_SIZE)
// 			sys_yield();

// 		// Escribir byte
// 		p->buffer[p->writeIndex] = data[i];
// 		p->writeIndex = (p->writeIndex + 1) % PIPE_BUFFER_SIZE;
// 		p->count++;
// 	}

// 	return size;
// }

// // Lee datos del pipe
// int pipe_read(int id, char *buffer, uint64_t size) {
// 	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
// 		return -1;

// 	Pipe *p = &pipes[id];

// 	for (uint64_t i = 0; i < size; i++) {
// 		// Esperar si el buffer está vacío
// 		while (p->count == 0)
// 			sys_yield();

// 		// Leer byte
// 		buffer[i] = p->buffer[p->readIndex];
// 		p->readIndex = (p->readIndex + 1) % PIPE_BUFFER_SIZE;
// 		p->count--;
// 	}

// 	return size;
// }

/////////////TODO: dejar esta version con semaforos/////////////7

#include "include/pipe.h"
#include "include/lib.h"
#include "include/stringKernel.h"
#include "scheduler/include/process.h"
#include "scheduler/include/semaphore.h"
// #include "include/memoryManager.h"

static Pipe pipes[MAX_PIPES];

void init_pipes() {
	for (int i = 0; i < MAX_PIPES; i++)
		pipes[i].active = 0;
}

static int find_pipe(char *name) {
	for (int i = 0; i < MAX_PIPES; i++)
		if (pipes[i].active && kstrcmp(pipes[i].name, name) == 0)
			return i;
	return -1;
}

int pipe_open(char *name) {
	int idx = find_pipe(name);
	if (idx >= 0) {
		pipes[idx].writers++;
		return idx;
	}

	// Buscar un slot libre
	for (int i = 0; i < MAX_PIPES; i++) {
		if (!pipes[i].active) {
			Pipe *p = &pipes[i];
			p->active = 1;
			kstrncpy(p->name, name, sizeof(p->name));
			p->readIndex = p->writeIndex = p->count = 0;
			p->readers = 1;
			p->writers = 1;
			p->sem_mutex = sem_open(name, 1);
			p->sem_read = sem_open("read", 0);
			p->sem_write = sem_open("write", PIPE_BUFFER_SIZE);
			return i;
		}
	}
	return -1;
}

int pipe_close(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];
	if (p->readers > 0)
		p->readers--;
	if (p->writers > 0)
		p->writers--;

	if (p->readers == 0 && p->writers == 0) {
		sem_close(p->sem_mutex);
		sem_close(p->sem_read);
		sem_close(p->sem_write);
		p->active = 0;
	}
	return 0;
}

int pipe_write(int id, const char *data, uint64_t size) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];
	for (uint64_t i = 0; i < size; i++) {
		sem_wait(p->sem_write);
		sem_wait(p->sem_mutex);

		p->buffer[p->writeIndex] = data[i];
		p->writeIndex = (p->writeIndex + 1) % PIPE_BUFFER_SIZE;
		p->count++;

		sem_post(p->sem_mutex);
		sem_post(p->sem_read);
	}
	return size;
}

int pipe_read(int id, char *buffer, uint64_t size) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];
	for (uint64_t i = 0; i < size; i++) {
		sem_wait(p->sem_read);
		sem_wait(p->sem_mutex);

		buffer[i] = p->buffer[p->readIndex];
		p->readIndex = (p->readIndex + 1) % PIPE_BUFFER_SIZE;
		p->count--;

		sem_post(p->sem_mutex);
		sem_post(p->sem_write);
	}
	return size;
}
