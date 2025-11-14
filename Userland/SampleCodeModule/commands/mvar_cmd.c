// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>
#include <stdint.h>

#define MVAR_MAX_NAME 32
#define MVAR_MAX_PARTICIPANTS 16
#define MVAR_MIN_DELAY 2000U
#define MVAR_DELAY_RANGE 8000U

typedef struct {
	int pipe_id;
	char sem_empty[MVAR_MAX_NAME];
	char sem_full[MVAR_MAX_NAME];
	int total_writers;
	int total_readers;
	volatile int writer_slot_state[MVAR_MAX_PARTICIPANTS];
	volatile int reader_slot_state[MVAR_MAX_PARTICIPANTS];
	int writer_pids[MVAR_MAX_PARTICIPANTS];
	int reader_pids[MVAR_MAX_PARTICIPANTS];
} MVarShared;

static const int reader_colors[] = {
	0x04, // rojo
	0x02, // verde
	0x01, // azul
	0x06, // marron
	0x05, // magenta
	0x03  // cian
};

static uint64_t string_to_u64(const char *str);

static void u64_to_string(uint64_t value, char *buffer, int buffer_size) {
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

	char temp[32];
	int temp_pos = 0;
	while (value > 0 && temp_pos < (int) sizeof(temp)) {
		temp[temp_pos++] = (char) ('0' + (value % 10));
		value /= 10;
	}

	int out_pos = 0;
	while (temp_pos > 0 && out_pos < buffer_size - 1) {
		buffer[out_pos++] = temp[--temp_pos];
	}
	buffer[out_pos] = '\0';
}

static uint64_t string_to_u64(const char *str) {
	if (str == NULL) {
		return 0;
	}

	uint64_t value = 0;
	int idx = 0;
	while (str[idx] != '\0') {
		char ch = str[idx];
		if (ch < '0' || ch > '9') {
			break;
		}
		value = value * 10 + (uint64_t) (ch - '0');
		idx++;
	}
	return value;
}

static void uint_to_string(unsigned value, char *buffer, int buffer_size) {
	if (buffer == NULL || buffer_size <= 0) {
		return;
	}

	char temp[16];
	int temp_pos = 0;
	if (value == 0) {
		if (buffer_size > 1) {
			buffer[0] = '0';
			buffer[1] = '\0';
		}
		return;
	}

	while (value > 0 && temp_pos < (int) sizeof(temp)) {
		temp[temp_pos++] = (char) ('0' + (value % 10));
		value /= 10;
	}

	int out_pos = 0;
	while (temp_pos > 0 && out_pos < buffer_size - 1) {
		buffer[out_pos++] = temp[--temp_pos];
	}
	buffer[out_pos] = '\0';
}

static void build_pipe_name(char *buffer, int buffer_size, int base_pid) {
	if (buffer == NULL || buffer_size <= 0) {
		return;
	}

	int pos = 0;
	const char prefix[] = "mvar_";
	for (int i = 0; prefix[i] != '\0' && pos < buffer_size - 1; i++) {
		buffer[pos++] = prefix[i];
	}

	char pid_buffer[16];
	uint_to_string((unsigned) base_pid, pid_buffer, (int) sizeof(pid_buffer));
	for (int i = 0; pid_buffer[i] != '\0' && pos < buffer_size - 1; i++) {
		buffer[pos++] = pid_buffer[i];
	}

	buffer[pos] = '\0';
}

static void build_sem_name(char *buffer, int buffer_size, int base_pid, const char *suffix) {
	if (buffer == NULL || buffer_size <= 0 || suffix == NULL) {
		return;
	}

	int pos = 0;
	const char prefix[] = "mvar_";
	for (int i = 0; prefix[i] != '\0' && pos < buffer_size - 1; i++) {
		buffer[pos++] = prefix[i];
	}

	char pid_buffer[16];
	uint_to_string((unsigned) base_pid, pid_buffer, (int) sizeof(pid_buffer));
	for (int i = 0; pid_buffer[i] != '\0' && pos < buffer_size - 1; i++) {
		buffer[pos++] = pid_buffer[i];
	}

	for (int i = 0; suffix[i] != '\0' && pos < buffer_size - 1; i++) {
		buffer[pos++] = suffix[i];
	}

	buffer[pos] = '\0';
}

static void random_delay(void) {
	uint32_t delay = GetUniform(MVAR_DELAY_RANGE) + MVAR_MIN_DELAY;
	bussy_wait(delay);
}

static void mvar_writer_worker(int argc, char **argv) {
	if (argc < 3 || argv == NULL || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL) {
		return;
	}

	MVarShared *shared = (MVarShared *) (uintptr_t) string_to_u64(argv[0]);
	if (shared == NULL) {
		return;
	}

	// Abrir el pipe por nombre para obtener el pipe_id
	// Esto asegura que el proceso tenga acceso al pipe incluso si el proceso principal termino
	char pipe_name[MVAR_MAX_NAME];
	int pos = 0;
	const char prefix[] = "mvar_";
	for (int i = 0; prefix[i] != '\0' && pos < MVAR_MAX_NAME - 1; i++) {
		pipe_name[pos++] = prefix[i];
	}
	for (int i = 0; argv[2][i] != '\0' && pos < MVAR_MAX_NAME - 1; i++) {
		pipe_name[pos++] = argv[2][i];
	}
	pipe_name[pos] = '\0';

	int64_t pipe_id = my_pipe_open(pipe_name);
	if (pipe_id < 0) {
		return;
	}

	int index = (int) satoi(argv[1]);
	if (index < 0) {
		index = 0;
	}

	if (index >= shared->total_writers && shared->total_writers > 0) {
		index = index % shared->total_writers;
	}

	char letter = (char) ('A' + (index % 26));

	while (1) {
		random_delay();

		// Esperar a que la MVar este vacia (sem_empty inicializado en 1)
		if (sem_wait(shared->sem_empty) != 0) {
			continue;
		}

		shared->writer_slot_state[index] = 1;

		// Escribir el valor al pipe
		if (my_pipe_write((uint64_t) pipe_id, &letter, 1) < 0) {
			shared->writer_slot_state[index] = 0;
			sem_post(shared->sem_empty);
			continue;
		}

		// Notificar que la MVar tiene un valor (sem_full inicializado en 0)
		if (sem_post(shared->sem_full) != 0) {
			shared->writer_slot_state[index] = 0;
			sem_post(shared->sem_empty);
			continue;
		}

		shared->writer_slot_state[index] = 0;
	}
}

static void mvar_reader_worker(int argc, char **argv) {
	if (argc < 3 || argv == NULL || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL) {
		return;
	}

	MVarShared *shared = (MVarShared *) (uintptr_t) string_to_u64(argv[0]);
	if (shared == NULL) {
		return;
	}

	// Abrir el pipe por nombre para obtener el pipe_id
	// Esto asegura que el proceso tenga acceso al pipe incluso si el proceso principal termino
	char pipe_name[MVAR_MAX_NAME];
	int pos = 0;
	const char prefix[] = "mvar_";
	for (int i = 0; prefix[i] != '\0' && pos < MVAR_MAX_NAME - 1; i++) {
		pipe_name[pos++] = prefix[i];
	}
	for (int i = 0; argv[2][i] != '\0' && pos < MVAR_MAX_NAME - 1; i++) {
		pipe_name[pos++] = argv[2][i];
	}
	pipe_name[pos] = '\0';

	int64_t pipe_id = my_pipe_open(pipe_name);
	if (pipe_id < 0) {
		return;
	}

	int index = (int) satoi(argv[1]);
	if (index < 0) {
		index = 0;
	}

	if (index >= shared->total_readers && shared->total_readers > 0) {
		index = index % shared->total_readers;
	}

	const int colors_count = (int) (sizeof(reader_colors) / sizeof(reader_colors[0]));
	int color = reader_colors[index % colors_count];

	while (1) {
		random_delay();

		// Esperar a que la MVar tenga un valor (sem_full inicializado en 0)
		if (sem_wait(shared->sem_full) != 0) {
			continue;
		}

		shared->reader_slot_state[index] = 1;

		// Leer el valor del pipe
		char value;
		if (my_pipe_read((uint64_t) pipe_id, &value, 1) <= 0) {
			shared->reader_slot_state[index] = 0;
			sem_post(shared->sem_full);
			continue;
		}

		shared->reader_slot_state[index] = 2;

		// Notificar que la MVar esta vacia (sem_empty inicializado en 1)
		if (sem_post(shared->sem_empty) != 0) {
			shared->reader_slot_state[index] = 0;
			continue;
		}

		putCharColor(value, color, 0);
		shared->reader_slot_state[index] = 0;
	}
}

static void mvar_main(int argc, char **argv) {
	if (argc < 3 || argv == NULL || argv[1] == NULL || argv[2] == NULL) {
		print_format("Usage: mvar <writers> <readers>\n");
		return;
	}

	int writers = validate_non_negative_int("mvar", "writers", argc, argv, 1);
	if (writers <= 0) {
		print_format("ERROR: writers must be greater than 0\n");
		return;
	}
	if (writers > MVAR_MAX_PARTICIPANTS) {
		print_format("ERROR: writers limit exceeded (max %d)\n", MVAR_MAX_PARTICIPANTS);
		return;
	}

	int readers = validate_non_negative_int("mvar", "readers", argc, argv, 2);
	if (readers <= 0) {
		print_format("ERROR: readers must be greater than 0\n");
		return;
	}
	if (readers > MVAR_MAX_PARTICIPANTS) {
		print_format("ERROR: readers limit exceeded (max %d)\n", MVAR_MAX_PARTICIPANTS);
		return;
	}

	MVarShared *shared = (MVarShared *) malloc(sizeof(MVarShared));
	if (shared == NULL) {
		print_format("ERROR: unable to allocate shared state for mvar\n");
		return;
	}

	shared->total_writers = writers;
	shared->total_readers = readers;
	for (int i = 0; i < MVAR_MAX_PARTICIPANTS; i++) {
		shared->writer_slot_state[i] = 0;
		shared->reader_slot_state[i] = 0;
		shared->writer_pids[i] = -1;
		shared->reader_pids[i] = -1;
	}

	int base_pid = getpid();
	char pipe_name[MVAR_MAX_NAME];
	build_pipe_name(pipe_name, MVAR_MAX_NAME, base_pid);

	int64_t pipe_id = my_pipe_open(pipe_name);
	if (pipe_id < 0) {
		print_format("ERROR: pipe_open failed for mvar pipe\n");
		free(shared);
		return;
	}

	shared->pipe_id = (int) pipe_id;

	// Crear semaforos para sincronizacion MVar
	// sem_empty: inicializado en 1 (MVar esta vacia, escritores pueden escribir)
	// sem_full: inicializado en 0 (MVar no tiene valor, lectores esperan)
	build_sem_name(shared->sem_empty, MVAR_MAX_NAME, base_pid, "_empty");
	build_sem_name(shared->sem_full, MVAR_MAX_NAME, base_pid, "_full");

	if (sem_open(shared->sem_empty, 1) < 0) {
		print_format("ERROR: sem_open failed for mvar empty semaphore\n");
		my_pipe_close((uint64_t) pipe_id);
		free(shared);
		return;
	}

	if (sem_open(shared->sem_full, 0) < 0) {
		print_format("ERROR: sem_open failed for mvar full semaphore\n");
		sem_close(shared->sem_empty);
		my_pipe_close((uint64_t) pipe_id);
		free(shared);
		return;
	}

	char context_arg[32];
	u64_to_string((uint64_t) (uintptr_t) shared, context_arg, (int) sizeof(context_arg));

	char pid_arg[16];
	uint_to_string((unsigned) base_pid, pid_arg, (int) sizeof(pid_arg));

	// Pre-allocar arrays para los argumentos de cada worker para evitar problemas de memoria
	char writer_index_args[MVAR_MAX_PARTICIPANTS][12];
	char reader_index_args[MVAR_MAX_PARTICIPANTS][12];
	// Los nombres deben estar en el stack (rango 0x400000-0x600000) para que el kernel los valide
	char writer_names[MVAR_MAX_PARTICIPANTS][32];
	char reader_names[MVAR_MAX_PARTICIPANTS][32];

	for (int i = 0; i < writers; i++) {
		uint_to_string((unsigned) i, writer_index_args[i], (int) sizeof(writer_index_args[i]));

		// Construir nombre del writer en el stack
		int name_pos = 0;
		const char worker_prefix[] = "mvar_w";
		while (worker_prefix[name_pos] != '\0' && name_pos < 31) {
			writer_names[i][name_pos] = worker_prefix[name_pos];
			name_pos++;
		}
		for (int j = 0; writer_index_args[i][j] != '\0' && name_pos < 31; j++) {
			writer_names[i][name_pos++] = writer_index_args[i][j];
		}
		writer_names[i][name_pos] = '\0';
	}

	for (int i = 0; i < readers; i++) {
		uint_to_string((unsigned) i, reader_index_args[i], (int) sizeof(reader_index_args[i]));

		// Construir nombre del reader en el stack
		int name_pos = 0;
		const char worker_prefix[] = "mvar_r";
		while (worker_prefix[name_pos] != '\0' && name_pos < 31) {
			reader_names[i][name_pos] = worker_prefix[name_pos];
			name_pos++;
		}
		for (int j = 0; reader_index_args[i][j] != '\0' && name_pos < 31; j++) {
			reader_names[i][name_pos++] = reader_index_args[i][j];
		}
		reader_names[i][name_pos] = '\0';
	}

	for (int i = 0; i < writers; i++) {
		char *worker_args[] = {context_arg, writer_index_args[i], pid_arg, NULL};

		int pid = create_process(writer_names[i], mvar_writer_worker, 3, worker_args, 128);
		if (pid < 0) {
			print_format("ERROR: failed to create writer %d\n", i);
		}
	}

	for (int i = 0; i < readers; i++) {
		char *worker_args[] = {context_arg, reader_index_args[i], pid_arg, NULL};

		int pid = create_process(reader_names[i], mvar_reader_worker, 3, worker_args, 128);
		if (pid < 0) {
			print_format("ERROR: failed to create reader %d\n", i);
		}
	}

	print_format("mvar: started %d writers and %d readers\n", writers, readers);
}

void mvar_cmd(int argc, char **argv) {
	int pid = command_spawn_process("mvar", mvar_main, argc, argv, 1, NULL);
	if (!validate_create_process_error("mvar", pid)) {
		return;
	}

	command_handle_child_process(pid, "mvar");
}
