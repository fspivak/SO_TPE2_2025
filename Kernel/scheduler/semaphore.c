// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/semaphore.h"
#include "../include/interrupts.h"
#include "include/process.h"
#include <stddef.h>

static sem_t sem_table[MAX_SEMAPHORES];
static volatile int initialized = 0;

static void init_semaphores() {
	if (initialized)
		return;

	_cli();
	if (initialized) {
		_sti();
		return;
	}

	for (int i = 0; i < MAX_SEMAPHORES; i++) {
		sem_table[i].name[0] = '\0';
		sem_table[i].value = 0;
		sem_table[i].head = 0;
		sem_table[i].tail = 0;
		sem_table[i].count = 0;
		sem_table[i].state = SEM_FREE;
		sem_table[i].users = 0;
		for (int j = 0; j < MAX_PROCESSES; j++) {
			sem_table[i].waiting_processes[j] = -1;
		}
	}
	initialized = 1;
	_sti();
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

static int validate_and_copy_name(const char *name, char *name_buffer) {
	if (name == NULL || name_buffer == NULL)
		return -1;

	int name_len = 0;
	int found_null = 0;

	while (name_len < MAX_SEM_NAME - 1) {
		volatile char c = *(volatile char *) (name + name_len);
		if (c == '\0') {
			found_null = 1;
			break;
		}
		if (c < 32 || c > 126) {
			return -1;
		}
		name_buffer[name_len] = c;
		name_len++;
	}

	if (!found_null) {
		if (name_len >= MAX_SEM_NAME - 1) {
			volatile char c = *(volatile char *) (name + name_len);
			if (c != '\0') {
				return -1;
			}
		}
	}

	name_buffer[name_len] = '\0';

	if (name_len == 0)
		return -1;

	return 0;
}

static int create_semaphore(const char *name, uint32_t initial_value) {
	char name_buffer[MAX_SEM_NAME];
	if (validate_and_copy_name(name, name_buffer) != 0)
		return SEM_NOT_FOUND;

	for (int i = 0; i < MAX_SEMAPHORES; i++) {
		if (sem_table[i].state == SEM_FREE) {
			int j = 0;
			while (name_buffer[j] != '\0' && j < MAX_SEM_NAME - 1) {
				sem_table[i].name[j] = name_buffer[j];
				j++;
			}
			sem_table[i].name[j] = '\0';
			sem_table[i].value = initial_value;
			sem_table[i].head = 0;
			sem_table[i].tail = 0;
			sem_table[i].count = 0;
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
	if (validate_and_copy_name(name, name_buffer) != 0)
		return -1;

	_cli();
	int sem_id = find_semaphore(name_buffer);
	if (sem_id != SEM_NOT_FOUND) {
		if (sem_table[sem_id].users < 65535) {
			sem_table[sem_id].users++;
		}
		_sti();
		return sem_id;
	}

	sem_id = create_semaphore(name_buffer, initial_value);
	_sti();
	return (sem_id == SEM_NOT_FOUND) ? -1 : sem_id;
}

int sem_wait(const char *name) {
	if (!initialized)
		init_semaphores();
	if (name == NULL)
		return -1;

	char name_buffer[MAX_SEM_NAME];
	if (validate_and_copy_name(name, name_buffer) != 0)
		return -1;

	_cli();
	int sem_id = find_semaphore(name_buffer);
	if (sem_id == SEM_NOT_FOUND) {
		_sti();
		return -1;
	}

	if (sem_table[sem_id].value > 0) {
		sem_table[sem_id].value--;
		_sti();
		return 0;
	}

	process_id_t current_pid = get_current_pid();
	if (current_pid < 0) {
		_sti();
		return -1;
	}

	if (sem_table[sem_id].count >= MAX_PROCESSES) {
		_sti();
		return -1;
	}

	sem_table[sem_id].waiting_processes[sem_table[sem_id].tail] = current_pid;
	sem_table[sem_id].tail = (sem_table[sem_id].tail + 1) % MAX_PROCESSES;
	sem_table[sem_id].count++;

	block_process(current_pid);
	_sti();

	return 0;
}

int sem_post(const char *name) {
	if (!initialized)
		init_semaphores();
	if (name == NULL)
		return -1;

	char name_buffer[MAX_SEM_NAME];
	if (validate_and_copy_name(name, name_buffer) != 0)
		return -1;

	_cli();
	int sem_id = find_semaphore(name_buffer);
	if (sem_id == SEM_NOT_FOUND) {
		_sti();
		return -1;
	}

	if (sem_table[sem_id].count > 0) {
		process_id_t pid_to_wake = -1;
		int checked = 0;

		while (checked < sem_table[sem_id].count) {
			pid_to_wake = sem_table[sem_id].waiting_processes[sem_table[sem_id].head];
			if (pid_to_wake >= 0) {
				sem_table[sem_id].waiting_processes[sem_table[sem_id].head] = -1;
				sem_table[sem_id].head = (sem_table[sem_id].head + 1) % MAX_PROCESSES;
				sem_table[sem_id].count--;
				_sti();

				unblock_process(pid_to_wake);
				_force_scheduler_interrupt();
				return 0;
			}

			sem_table[sem_id].head = (sem_table[sem_id].head + 1) % MAX_PROCESSES;
			checked++;
		}

		sem_table[sem_id].count = 0;
		sem_table[sem_id].head = 0;
		sem_table[sem_id].tail = 0;
		_sti();
		return 0;
	}

	sem_table[sem_id].value++;
	_sti();
	return 0;
}

int sem_close(const char *name) {
	if (!initialized)
		init_semaphores();
	if (name == NULL)
		return -1;

	char name_buffer[MAX_SEM_NAME];
	if (validate_and_copy_name(name, name_buffer) != 0)
		return -1;

	_cli();
	int sem_id = find_semaphore(name_buffer);
	if (sem_id == SEM_NOT_FOUND) {
		_sti();
		return -1;
	}

	if (sem_table[sem_id].users > 0) {
		sem_table[sem_id].users--;
	}

	int wake_count = 0;
	process_id_t pids_to_wake[MAX_PROCESSES];

	if (sem_table[sem_id].users == 0) {
		wake_count = sem_table[sem_id].count;
		for (int i = 0; i < wake_count; i++) {
			int idx = (sem_table[sem_id].head + i) % MAX_PROCESSES;
			pids_to_wake[i] = sem_table[sem_id].waiting_processes[idx];
			sem_table[sem_id].waiting_processes[idx] = -1;
		}

		sem_table[sem_id].state = SEM_FREE;
		sem_table[sem_id].name[0] = '\0';
		sem_table[sem_id].value = 0;
		sem_table[sem_id].head = 0;
		sem_table[sem_id].tail = 0;
		sem_table[sem_id].count = 0;
	}
	_sti();

	if (wake_count > 0) {
		for (int i = 0; i < wake_count; i++) {
			if (pids_to_wake[i] >= 0) {
				unblock_process(pids_to_wake[i]);
			}
		}
		_force_scheduler_interrupt();
	}

	return 0;
}

int sem_get_waiting_count(const char *name) {
	if (!initialized)
		init_semaphores();
	if (name == NULL)
		return -1;

	char name_buffer[MAX_SEM_NAME];
	if (validate_and_copy_name(name, name_buffer) != 0)
		return -1;

	_cli();
	int sem_id = find_semaphore(name_buffer);
	if (sem_id == SEM_NOT_FOUND) {
		_sti();
		return -1;
	}

	int count = sem_table[sem_id].count;
	_sti();
	return count;
}
