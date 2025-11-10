
#include "include/pipe.h"
#include "include/lib.h"
#include "include/stringKernel.h"
#include "scheduler/include/process.h"
#include "scheduler/include/semaphore.h"

#define PIPE_BUFFER_SIZE 256

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

	for (int i = 0; i < MAX_PIPES; i++) {
		if (!pipes[i].active) {
			Pipe *p = &pipes[i];
			p->active = 1;
			kstrncpy(p->name, name, sizeof(p->name));
			p->readIndex = p->writeIndex = p->count = 0;
			p->readers = 0;
			p->writers = 1;
			p->eof = 0;

			char sem_mutex_name[48];
			char sem_read_name[48];
			char sem_write_name[48];
			char num[8];

			// número como string
			int temp = i, pos = 0;
			if (temp == 0)
				num[pos++] = '0';
			else {
				char rev[8];
				int rpos = 0;
				while (temp > 0 && rpos < 8) {
					rev[rpos++] = '0' + (temp % 10);
					temp /= 10;
				}
				while (rpos > 0)
					num[pos++] = rev[--rpos];
			}
			num[pos] = '\0';

			kstrcpy(sem_mutex_name, name);
			kstrcat(sem_mutex_name, "_mutex_");
			kstrcat(sem_mutex_name, num);

			kstrcpy(sem_read_name, name);
			kstrcat(sem_read_name, "_read_");
			kstrcat(sem_read_name, num);

			kstrcpy(sem_write_name, name);
			kstrcat(sem_write_name, "_write_");
			kstrcat(sem_write_name, num);

			p->sem_mutex = sem_open(sem_mutex_name, 1);
			p->sem_read = sem_open(sem_read_name, 1); // inicial 1 para evitar deadlock inicial
			p->sem_write = sem_open(sem_write_name, PIPE_BUFFER_SIZE);
			return i;
		}
	}
	return -1;
}

int pipe_close(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];
	int last_writer = 0;

	sem_wait(p->sem_mutex);

	// Cerramos como writer por defecto (es lo que hace cat)
	if (p->writers > 0) {
		p->writers--;
		if (p->writers == 0) {
			p->eof = 1;
			last_writer = 1; // ← necesitamos despertar a los readers
		}
	}
	else if (p->readers > 0) {
		// (solo si alguna vez cerrás lectores explícitamente)
		p->readers--;
	}

	int kill = (p->readers == 0 && p->writers == 0);
	sem_post(p->sem_mutex);

	if (last_writer) {
		// 🔴 clave: despertar lectores bloqueados para que vean EOF y salgan
		sem_post(p->sem_read);
		// si podés tener varios readers bloqueados, posteá varias veces o
		// cambiá pipe_read para no quedarse esperando más de 1 byte.
	}

	if (kill) {
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

	sem_wait(p->sem_mutex);
	if (p->writers == 0)
		p->writers = 1;
	sem_post(p->sem_mutex);

	uint64_t written = 0;
	while (written < size) {
		sem_wait(p->sem_write);
		sem_wait(p->sem_mutex);

		p->buffer[p->writeIndex] = data[written++];
		p->writeIndex = (p->writeIndex + 1) % PIPE_BUFFER_SIZE;
		p->count++;

		sem_post(p->sem_mutex);
		sem_post(p->sem_read);
	}
	return written;
}

int pipe_read(int id, char *buffer, uint64_t size) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];

	sem_wait(p->sem_mutex);
	if (p->readers == 0)
		p->readers = 1;
	sem_post(p->sem_mutex);

	uint64_t read_bytes = 0;

	while (read_bytes < size) {
		sem_wait(p->sem_mutex);

		if (p->count > 0) {
			buffer[read_bytes++] = p->buffer[p->readIndex];
			p->readIndex = (p->readIndex + 1) % PIPE_BUFFER_SIZE;
			p->count--;
			sem_post(p->sem_mutex);
			sem_post(p->sem_write);
			continue;
		}

		// EOF real
		if (p->writers == 0) {
			p->eof = 1;
			sem_post(p->sem_mutex);
			return read_bytes;
		}

		sem_post(p->sem_mutex);
		sem_wait(p->sem_read);
	}
	return read_bytes;
}
