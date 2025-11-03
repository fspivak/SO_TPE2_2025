#include "include/semaphore.h"
#include "../include/interrupts.h"
#include "include/process.h"
#include <stddef.h>

static sem_t sem_table[MAX_SEMAPHORES];
static int initialized = 0;

static void init_semaphores() {
	if (initialized)
		return;

	for (int i = 0; i < MAX_SEMAPHORES; i++) {
		sem_table[i].name[0] = '\0';
		sem_table[i].value = 0;
		sem_table[i].current_index = 0;
		sem_table[i].last_index = 0;
		sem_table[i].state = SEM_FREE;
		sem_table[i].users = 0;
		for (int j = 0; j < MAX_PROCESSES; j++) {
			sem_table[i].waiting_processes[j] = -1;
		}
	}
	initialized = 1;
}

static int find_semaphore(const char *name) {
	if (name == NULL)
		return SEM_NOT_FOUND;

	for (int j = 0; j < MAX_SEMAPHORES; j++) {
		if (sem_table[j].state == SEM_USED) {
			int k = 0;
			int match = 1;
			while (k < MAX_SEM_NAME - 1) {
				char c1 = sem_table[j].name[k];
				char c2 = name[k];

				if (c1 == '\0' && c2 == '\0') {
					return j;
				}

				if (c1 != c2) {
					match = 0;
					break;
				}

				if (c1 == '\0' || c2 == '\0') {
					match = 0;
					break;
				}

				k++;
			}
			if (match && sem_table[j].name[k] == '\0' && name[k] == '\0') {
				return j;
			}
		}
	}
	return SEM_NOT_FOUND;
}

static int create_semaphore(const char *name, uint32_t initial_value) {
	char name_buffer[MAX_SEM_NAME];
	int name_len = 0;
	while (name_len < MAX_SEM_NAME - 1 && name[name_len] != '\0') {
		name_buffer[name_len] = name[name_len];
		name_len++;
	}
	name_buffer[name_len] = '\0';

	for (int i = 0; i < MAX_SEMAPHORES; i++) {
		if (sem_table[i].state == SEM_FREE) {
			int j = 0;
			while (j <= name_len && j < MAX_SEM_NAME - 1) {
				sem_table[i].name[j] = name_buffer[j];
				j++;
			}
			sem_table[i].name[j] = '\0';
			sem_table[i].value = initial_value;
			sem_table[i].current_index = 0;
			sem_table[i].last_index = 0;
			sem_table[i].state = SEM_USED;
			sem_table[i].users = 1;
			for (int k = 0; k < MAX_PROCESSES; k++) {
				sem_table[i].waiting_processes[k] = -1;
			}
			return i;
		}
	}
	return SEM_NOT_FOUND;
}

int sem_open(const char *name, uint32_t initial_value) {
	if (!initialized)
		init_semaphores();
	if (name == NULL)
		return -1;

	char name_buffer[MAX_SEM_NAME];
	int name_len = 0;

	while (name_len < MAX_SEM_NAME - 1) {
		volatile char c = *(volatile char *) (name + name_len);
		if (c == '\0') {
			break;
		}
		if (c < 32 || c > 126) {
			return -1;
		}
		name_buffer[name_len] = c;
		name_len++;
	}
	name_buffer[name_len] = '\0';

	if (name_len == 0)
		return -1;

	int sem_id = find_semaphore(name_buffer);
	if (sem_id != SEM_NOT_FOUND) {
		sem_table[sem_id].users++;
		return 0;
	}

	sem_id = find_semaphore(name_buffer);
	if (sem_id != SEM_NOT_FOUND) {
		sem_table[sem_id].users++;
		return 0;
	}

	sem_id = create_semaphore(name_buffer, initial_value);
	return (sem_id == SEM_NOT_FOUND) ? -1 : 0;
}

int sem_wait(const char *name) {
	if (!initialized)
		init_semaphores();
	if (name == NULL)
		return -1;

	char name_buffer[MAX_SEM_NAME];
	int i = 0;
	while (i < MAX_SEM_NAME - 1 && name[i] != '\0') {
		name_buffer[i] = name[i];
		i++;
	}
	name_buffer[i] = '\0';

	int sem_id = find_semaphore(name_buffer);
	if (sem_id == SEM_NOT_FOUND)
		return -1;

	if (sem_table[sem_id].value > 0) {
		sem_table[sem_id].value--;
		return 0;
	}

	process_id_t current_pid = get_current_pid();
	if (current_pid < 0)
		return -1;

	if (sem_table[sem_id].last_index >= MAX_PROCESSES)
		return -1;
	sem_table[sem_id].waiting_processes[sem_table[sem_id].last_index] = current_pid;
	sem_table[sem_id].last_index++;

	block_process(current_pid);

	extern void _force_scheduler_interrupt();
	_force_scheduler_interrupt();

	return 0;
}

int sem_post(const char *name) {
	if (!initialized)
		init_semaphores();
	if (name == NULL)
		return -1;

	char name_buffer[MAX_SEM_NAME];
	int i = 0;
	while (i < MAX_SEM_NAME - 1 && name[i] != '\0') {
		name_buffer[i] = name[i];
		i++;
	}
	name_buffer[i] = '\0';

	int sem_id = find_semaphore(name_buffer);
	if (sem_id == SEM_NOT_FOUND)
		return -1;

	if (sem_table[sem_id].current_index < sem_table[sem_id].last_index) {
		process_id_t pid_to_wake = sem_table[sem_id].waiting_processes[sem_table[sem_id].current_index];
		if (pid_to_wake >= 0) {
			sem_table[sem_id].waiting_processes[sem_table[sem_id].current_index] = -1;
			sem_table[sem_id].current_index++;
			unblock_process(pid_to_wake);
			extern void _force_scheduler_interrupt();
			_force_scheduler_interrupt();
			return 0;
		}
	}

	sem_table[sem_id].value++;
	return 0;
}

int sem_close(const char *name) {
	if (!initialized)
		init_semaphores();
	if (name == NULL)
		return -1;

	char name_buffer[MAX_SEM_NAME];
	int i = 0;
	while (i < MAX_SEM_NAME - 1 && name[i] != '\0') {
		name_buffer[i] = name[i];
		i++;
	}
	name_buffer[i] = '\0';

	int sem_id = find_semaphore(name_buffer);
	if (sem_id == SEM_NOT_FOUND)
		return -1;

	sem_table[sem_id].users--;

	if (sem_table[sem_id].users == 0) {
		sem_table[sem_id].state = SEM_FREE;
		sem_table[sem_id].name[0] = '\0';
		sem_table[sem_id].value = 0;
		sem_table[sem_id].current_index = 0;
		sem_table[sem_id].last_index = 0;
	}

	return 0;
}
