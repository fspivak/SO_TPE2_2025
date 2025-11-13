#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/test_util.h"
#include <stddef.h>
#include <stdint.h>

#define MVAR_MAX_NAME 32
#define MVAR_MAX_PARTICIPANTS 16
#define MVAR_MIN_DELAY 2000U
#define MVAR_DELAY_RANGE 8000U

typedef struct {
	volatile char value;
	volatile int has_value;
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
static void mvar_guard_loop(int argc, char **argv);

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
	if (argc < 2 || argv == NULL || argv[0] == NULL || argv[1] == NULL) {
		return;
	}

	MVarShared *shared = (MVarShared *) (uintptr_t) string_to_u64(argv[0]);
	if (shared == NULL) {
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

		if (sem_wait(shared->sem_empty) != 0) {
			continue;
		}

		shared->writer_slot_state[index] = 1;
		shared->value = letter;

		if (sem_post(shared->sem_full) != 0) {
			shared->writer_slot_state[index] = 0;
			sem_post(shared->sem_empty);
			continue;
		}

		shared->has_value = 1;
		shared->writer_slot_state[index] = 0;
	}
}

static void mvar_reader_worker(int argc, char **argv) {
	if (argc < 2 || argv == NULL || argv[0] == NULL || argv[1] == NULL) {
		return;
	}

	MVarShared *shared = (MVarShared *) (uintptr_t) string_to_u64(argv[0]);
	if (shared == NULL) {
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

		if (sem_wait(shared->sem_full) != 0) {
			continue;
		}

		shared->reader_slot_state[index] = 1;
		char value = shared->value;
		shared->reader_slot_state[index] = 2;
		shared->has_value = 0;

		if (sem_post(shared->sem_empty) != 0) {
			continue;
		}

		shared->reader_slot_state[index] = 0;
		putCharColor(value, color, 0);
	}
}

static int pid_is_alive(int pid, ProcessInfo *processes, int count) {
	if (pid <= 0 || processes == NULL) {
		return 0;
	}

	for (int i = 0; i < count; i++) {
		if (processes[i].pid == pid) {
			if (strcmp(processes[i].state_name, "TERMINATED") == 0) {
				return 0;
			}
			return 1;
		}
	}
	return 0;
}

static void release_writer_slot(MVarShared *shared, int index) {
	int posted = 0;
	if (shared->writer_slot_state[index] != 0) {
		sem_post(shared->sem_empty);
		posted = 1;
	}

	shared->writer_slot_state[index] = 0;

	int other_holding = 0;
	for (int i = 0; i < shared->total_writers; i++) {
		if (i == index) {
			continue;
		}
		if (shared->writer_slot_state[i] != 0) {
			other_holding = 1;
			break;
		}
	}

	if (!other_holding && shared->has_value == 0 && !posted) {
		sem_post(shared->sem_empty);
	}
}

static void release_reader_slot(MVarShared *shared, int index) {
	if (shared->reader_slot_state[index] == 1) {
		sem_post(shared->sem_full);
	}
	else if (shared->reader_slot_state[index] == 2) {
		sem_post(shared->sem_empty);
	}
	shared->reader_slot_state[index] = 0;
}

static void release_pending_value_if_needed(MVarShared *shared, int alive_writer_count) {
	if (shared->has_value != 0) {
		if (alive_writer_count > 0) {
			sem_post(shared->sem_empty);
		}
		shared->has_value = 0;
	}
}

static void mvar_guard_loop(int argc, char **argv) {
	if (argc < 3 || argv == NULL || argv[0] == NULL || argv[1] == NULL || argv[2] == NULL) {
		exit_process();
		return;
	}

	MVarShared *shared = (MVarShared *) (uintptr_t) string_to_u64(argv[0]);
	if (shared == NULL) {
		exit_process();
		return;
	}

	int total_writers = satoi(argv[1]);
	int total_readers = satoi(argv[2]);
	if (total_writers < 0) {
		total_writers = 0;
	}
	if (total_readers < 0) {
		total_readers = 0;
	}

	ProcessInfo processes[64];

	int active_writers = 0;
	int active_readers = 0;

	for (int i = 0; i < total_writers; i++) {
		char index_arg[12];
		uint_to_string((unsigned) i, index_arg, (int) sizeof(index_arg));

		char context_arg[32];
		u64_to_string((uint64_t) (uintptr_t) shared, context_arg, (int) sizeof(context_arg));

		char *worker_args[] = {context_arg, index_arg, NULL};

		char worker_name[32];
		int name_pos = 0;
		const char worker_prefix[] = "mvar_w";
		while (worker_prefix[name_pos] != '\0' && name_pos < (int) sizeof(worker_name) - 1) {
			worker_name[name_pos] = worker_prefix[name_pos];
			name_pos++;
		}
		for (int j = 0; index_arg[j] != '\0' && name_pos < (int) sizeof(worker_name) - 1; j++) {
			worker_name[name_pos++] = index_arg[j];
		}
		worker_name[name_pos] = '\0';

		int pid = create_process(worker_name, mvar_writer_worker, 2, worker_args, 128);
		if (pid < 0) {
			print_format("ERROR: failed to create writer %d\n", i);
			shared->writer_pids[i] = -1;
			continue;
		}

		shared->writer_pids[i] = pid;
		active_writers++;
	}

	for (int i = 0; i < total_readers; i++) {
		char index_arg[12];
		uint_to_string((unsigned) i, index_arg, (int) sizeof(index_arg));

		char context_arg[32];
		u64_to_string((uint64_t) (uintptr_t) shared, context_arg, (int) sizeof(context_arg));

		char *worker_args[] = {context_arg, index_arg, NULL};

		char worker_name[32];
		int name_pos = 0;
		const char worker_prefix[] = "mvar_r";
		while (worker_prefix[name_pos] != '\0' && name_pos < (int) sizeof(worker_name) - 1) {
			worker_name[name_pos] = worker_prefix[name_pos];
			name_pos++;
		}
		for (int j = 0; index_arg[j] != '\0' && name_pos < (int) sizeof(worker_name) - 1; j++) {
			worker_name[name_pos++] = index_arg[j];
		}
		worker_name[name_pos] = '\0';

		int pid = create_process(worker_name, mvar_reader_worker, 2, worker_args, 128);
		if (pid < 0) {
			print_format("ERROR: failed to create reader %d\n", i);
			shared->reader_pids[i] = -1;
			continue;
		}

		shared->reader_pids[i] = pid;
		active_readers++;
	}

	while (active_writers > 0 || active_readers > 0) {
		int count = ps(processes, 64);
		if (count < 0) {
			yield();
			continue;
		}

		int alive_writers_count = 0;

		for (int i = 0; i < total_writers; i++) {
			int pid = shared->writer_pids[i];
			if (pid <= 0) {
				continue;
			}

			if (!pid_is_alive(pid, processes, count)) {
				waitpid(pid);
				release_writer_slot(shared, i);
				shared->writer_pids[i] = -1;
				if (active_writers > 0) {
					active_writers--;
				}
			}
			else {
				alive_writers_count++;
			}
		}

		for (int i = 0; i < total_readers; i++) {
			int pid = shared->reader_pids[i];
			if (pid <= 0) {
				continue;
			}

			if (!pid_is_alive(pid, processes, count)) {
				waitpid(pid);
				release_reader_slot(shared, i);
				shared->reader_pids[i] = -1;
				if (active_readers > 0) {
					active_readers--;
				}
				release_pending_value_if_needed(shared, alive_writers_count);
			}
		}

		if (active_writers == 0 && active_readers == 0) {
			break;
		}

		yield();
	}

	sem_close(shared->sem_full);
	sem_close(shared->sem_empty);
	free(shared);
	exit_process();
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

	shared->value = 0;
	shared->has_value = 0;
	shared->total_writers = writers;
	shared->total_readers = readers;
	for (int i = 0; i < MVAR_MAX_PARTICIPANTS; i++) {
		shared->writer_slot_state[i] = 0;
		shared->reader_slot_state[i] = 0;
		shared->writer_pids[i] = -1;
		shared->reader_pids[i] = -1;
	}

	int base_pid = getpid();
	build_sem_name(shared->sem_empty, MVAR_MAX_NAME, base_pid, "_empty");
	build_sem_name(shared->sem_full, MVAR_MAX_NAME, base_pid, "_full");

	if (sem_open(shared->sem_empty, 1) < 0) {
		print_format("ERROR: sem_open failed for mvar empty semaphore\n");
		free(shared);
		return;
	}

	if (sem_open(shared->sem_full, 0) < 0) {
		print_format("ERROR: sem_open failed for mvar full semaphore\n");
		sem_close(shared->sem_empty);
		free(shared);
		return;
	}

	char context_arg[32];
	u64_to_string((uint64_t) (uintptr_t) shared, context_arg, (int) sizeof(context_arg));

	char writers_arg[12];
	uint_to_string((unsigned) writers, writers_arg, (int) sizeof(writers_arg));

	char readers_arg[12];
	uint_to_string((unsigned) readers, readers_arg, (int) sizeof(readers_arg));

	char *guard_args[] = {context_arg, writers_arg, readers_arg, NULL};

	if (create_process("mvar_guard", mvar_guard_loop, 3, guard_args, 1) < 0) {
		print_format("ERROR: failed to create mvar guard\n");
		sem_close(shared->sem_full);
		sem_close(shared->sem_empty);
		free(shared);
		return;
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
