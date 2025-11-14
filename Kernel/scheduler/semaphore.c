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
	// Double-check locking: verificar nuevamente dentro de seccion critica
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

	// Optimizacion: comparar nombres directamente con strcmp equivalente
	// pero sin usar funciones de biblioteca estandar
	for (int j = 0; j < MAX_SEMAPHORES; j++) {
		if (sem_table[j].state == SEM_USED) {
			int k = 0;
			// Comparar caracter por caracter hasta encontrar diferencia o fin de string
			while (k < MAX_SEM_NAME - 1) {
				char c1 = sem_table[j].name[k];
				char c2 = name[k];

				// Si ambos son null, encontramos match
				if (c1 == '\0' && c2 == '\0') {
					return j;
				}

				// Si uno es null y el otro no, no hay match
				if (c1 == '\0' || c2 == '\0') {
					break;
				}

				// Si son diferentes, no hay match
				if (c1 != c2) {
					break;
				}

				k++;
			}
			// Verificar match completo (ambos terminan en la misma posicion)
			// Si k < MAX_SEM_NAME - 1, el loop termino por encontrar null o diferencia
			// Si k == MAX_SEM_NAME - 1, el loop termino por el limite, verificar igualdad
			if (k <= MAX_SEM_NAME - 1 && sem_table[j].name[k] == '\0' && name[k] == '\0') {
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

	process_id_t current_pid = get_current_pid();
	if (current_pid < 0) {
		return -1;
	}

	while (1) {
		_cli();
		int sem_id = find_semaphore(name_buffer);
		if (sem_id == SEM_NOT_FOUND) {
			// El semaforo no existe (fue cerrado o nunca existio)
			// Si el proceso estaba en alguna cola, ya fue removido por sem_remove_process
			_sti();
			return -1;
		}

		if (sem_table[sem_id].value > 0) {
			// Hay recursos disponibles, tomarlo inmediatamente
			sem_table[sem_id].value--;
			_sti();
			return 0;
		}

		if (sem_table[sem_id].count >= MAX_PROCESSES) {
			_sti();
			return -1;
		}

		// Verificar si el proceso ya esta en la cola de espera
		// Esto evita agregar el mismo proceso multiples veces
		int process_in_queue = 0;
		for (int i = 0; i < sem_table[sem_id].count; i++) {
			int idx = (sem_table[sem_id].head + i) % MAX_PROCESSES;
			if (sem_table[sem_id].waiting_processes[idx] == current_pid) {
				process_in_queue = 1;
				break;
			}
		}

		if (process_in_queue) {
			// El proceso ya esta en la cola, solo bloquearlo
			// Si fue removido mientras estaba bloqueado, process_in_queue sera 0
			// y se agregara nuevamente a la cola
			_sti();
			if (block_process(current_pid) < 0) {
				// El proceso fue terminado mientras esperaba, salir
				return -1;
			}
			_force_scheduler_interrupt();
			continue;
		}

		// Agregar el proceso a la cola de espera
		sem_table[sem_id].waiting_processes[sem_table[sem_id].tail] = current_pid;
		sem_table[sem_id].tail = (sem_table[sem_id].tail + 1) % MAX_PROCESSES;
		sem_table[sem_id].count++;

		_sti();
		if (block_process(current_pid) < 0) {
			// El proceso fue terminado mientras se agregaba a la cola
			// Removerlo de la cola y retornar error
			_cli();
			// El proceso se agregó al final (tail), removerlo desde ahí
			int prev_tail = (sem_table[sem_id].tail - 1 + MAX_PROCESSES) % MAX_PROCESSES;
			if (sem_table[sem_id].waiting_processes[prev_tail] == current_pid) {
				sem_table[sem_id].waiting_processes[prev_tail] = -1;
				sem_table[sem_id].tail = prev_tail;
				sem_table[sem_id].count--;
			}
			else {
				// Buscar en toda la cola por si acaso
				for (int i = 0; i < sem_table[sem_id].count; i++) {
					int idx = (sem_table[sem_id].head + i) % MAX_PROCESSES;
					if (sem_table[sem_id].waiting_processes[idx] == current_pid) {
						sem_table[sem_id].waiting_processes[idx] = -1;
						// Compactar la cola moviendo elementos
						for (int j = i; j < sem_table[sem_id].count - 1; j++) {
							int curr = (sem_table[sem_id].head + j) % MAX_PROCESSES;
							int next = (sem_table[sem_id].head + j + 1) % MAX_PROCESSES;
							sem_table[sem_id].waiting_processes[curr] = sem_table[sem_id].waiting_processes[next];
						}
						sem_table[sem_id].tail = (sem_table[sem_id].tail - 1 + MAX_PROCESSES) % MAX_PROCESSES;
						sem_table[sem_id].count--;
						break;
					}
				}
			}
			_sti();
			return -1;
		}
		_force_scheduler_interrupt();
	}
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
		int processes_removed = 0;

		while (checked < sem_table[sem_id].count) {
			pid_to_wake = sem_table[sem_id].waiting_processes[sem_table[sem_id].head];
			if (pid_to_wake < 0) {
				// Slot vacio (proceso ya removido), limpiarlo y continuar
				sem_table[sem_id].head = (sem_table[sem_id].head + 1) % MAX_PROCESSES;
				sem_table[sem_id].count--;
				checked++;
				continue;
			}

			// Verificar si el proceso sigue vivo antes de despertarlo
			PCB *process = get_process_by_pid(pid_to_wake);
			if (process == NULL || process->state == TERMINATED || process->state != BLOCKED) {
				// Proceso muerto o no bloqueado, removerlo de la cola y continuar
				sem_table[sem_id].waiting_processes[sem_table[sem_id].head] = -1;
				sem_table[sem_id].head = (sem_table[sem_id].head + 1) % MAX_PROCESSES;
				sem_table[sem_id].count--;
				processes_removed++;
				checked++;
				continue;
			}

			// Proceso vivo y bloqueado, despertarlo
			sem_table[sem_id].waiting_processes[sem_table[sem_id].head] = -1;
			sem_table[sem_id].head = (sem_table[sem_id].head + 1) % MAX_PROCESSES;
			sem_table[sem_id].count--;
			// Incrementar el valor del semaforo para que el proceso desbloqueado pueda tomarlo
			sem_table[sem_id].value++;
			_sti();

			unblock_process(pid_to_wake);
			_force_scheduler_interrupt();
			return 0;
		}

		// Si la cola quedo vacia, limpiarla
		if (sem_table[sem_id].count == 0) {
			sem_table[sem_id].head = 0;
			sem_table[sem_id].tail = 0;
		}

		// IMPORTANTE: sem_post siempre debe incrementar el valor del semaforo
		// porque libera un recurso. Si todos los procesos estaban muertos,
		// el valor ya se incremento cuando se removieron, pero sem_post
		// debe incrementar el valor una vez mas para liberar el recurso
		// que el proceso que llama a sem_post esta liberando
		sem_table[sem_id].value++;
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
				// Verificar que el proceso este vivo antes de despertarlo
				PCB *process = get_process_by_pid(pids_to_wake[i]);
				if (process != NULL && process->state == BLOCKED) {
					unblock_process(pids_to_wake[i]);
				}
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

void sem_remove_process(process_id_t pid) {
	if (!initialized)
		init_semaphores();
	if (pid < 0)
		return;

	_cli();
	for (int sem_id = 0; sem_id < MAX_SEMAPHORES; sem_id++) {
		if (sem_table[sem_id].state != SEM_USED)
			continue;

		// Buscar el proceso en la cola y removerlo
		int removed = 0;
		int checked = 0;
		int original_count = sem_table[sem_id].count;

		while (checked < original_count) {
			int idx = (sem_table[sem_id].head + checked) % MAX_PROCESSES;
			process_id_t current_pid = sem_table[sem_id].waiting_processes[idx];

			if (current_pid == pid) {
				// Encontrado, removerlo
				sem_table[sem_id].waiting_processes[idx] = -1;
				sem_table[sem_id].head = (sem_table[sem_id].head + 1) % MAX_PROCESSES;
				sem_table[sem_id].count--;
				removed = 1;

				// Incrementar el valor del semaforo porque el proceso muerto no lo va a tomar
				sem_table[sem_id].value++;
				break;
			}
			checked++;
		}

		// Si no estaba en la cola de espera, el proceso podria haber tenido el semaforo
		// (lo obtuvo con sem_wait pero murio antes de hacer sem_post)
		// Si hay procesos esperando y el valor es 0, incrementar para evitar deadlock
		// Esta es una heuristica conservadora: solo incrementar si hay evidencia de deadlock
		if (!removed && sem_table[sem_id].value == 0 && sem_table[sem_id].count > 0) {
			// Hay procesos esperando pero el valor es 0, probable deadlock
			// Incrementar el valor para permitir que un proceso espere pueda continuar
			sem_table[sem_id].value++;
		}
	}
	_sti();
}
