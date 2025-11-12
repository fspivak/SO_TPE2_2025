
#include "include/pipe.h"
#include "include/lib.h"
#include "include/stringKernel.h"
#include "scheduler/include/process.h"
#include "scheduler/include/semaphore.h"
// #include "include/memoryManager.h"

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
			return i;
		}
	}
	return -1;
}

int pipe_close(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];
	if (p->readers > 0) {
		p->readers--;
	}
	if (p->writers > 0) {
		p->writers--;
	}
	pipe_destroy_if_unused(p);
	return 0;
}

int pipe_write(int id, const char *data, uint64_t size) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];
	for (uint64_t i = 0; i < size; i++) {
		if (sem_wait(p->sem_write_name) != 0) {
			return -1;
		}
		if (sem_wait(p->sem_mutex_name) != 0) {
			return -1;
		}

		p->buffer[p->writeIndex] = data[i];
		p->writeIndex = (p->writeIndex + 1) % PIPE_BUFFER_SIZE;
		p->count++;

		sem_post(p->sem_mutex_name);
		sem_post(p->sem_read_name);
	}
	return size;
}

int pipe_read(int id, char *buffer, uint64_t size) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active)
		return -1;

	Pipe *p = &pipes[id];
	for (uint64_t i = 0; i < size; i++) {
		if (sem_wait(p->sem_read_name) != 0) {
			return -1;
		}
		if (sem_wait(p->sem_mutex_name) != 0) {
			return -1;
		}

		buffer[i] = p->buffer[p->readIndex];
		p->readIndex = (p->readIndex + 1) % PIPE_BUFFER_SIZE;
		p->count--;

		sem_post(p->sem_mutex_name);
		sem_post(p->sem_write_name);
	}
	return size;
}

int pipe_register_reader(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active) {
		return -1;
	}
	pipes[id].readers++;
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
		return -1;
	}
	pipes[id].writers++;
	return 0;
}

int pipe_unregister_writer(int id) {
	if (id < 0 || id >= MAX_PIPES || !pipes[id].active) {
		return -1;
	}
	if (pipes[id].writers > 0) {
		pipes[id].writers--;
	}
	pipe_destroy_if_unused(&pipes[id]);
	return 0;
}
