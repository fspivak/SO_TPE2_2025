#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>
#include <stdint.h>

extern uint64_t test_sync(uint64_t argc, char *argv[]);

void test_sync_main(int argc, char **argv) {
	if (argc < 3 || argv == NULL || argv[1] == NULL || argv[2] == NULL) {
		print_format("test_sync: Usage: test_sync <n> <use_sem>\n");
		print_format("  n: Number of iterations (must be non-negative)\n");
		print_format("  use_sem: 1 to use semaphores, 0 to test without semaphores\n");
		print_format("  Example: test_sync 1000 1\n");
		return;
	}

	int n = validate_non_negative_int("test_sync", "n (iterations)", argc, argv, 1);
	if (n < 0) {
		return;
	}

	int use_sem = satoi(argv[2]);
	if (use_sem != 0 && use_sem != 1) {
		print_format("ERROR: use_sem must be 0 or 1\n");
		return;
	}

	char *args[] = {argv[1], argv[2], NULL};

	int64_t result = test_sync(2, args);

	if (result != 0) {
		print_format("test_sync: ERROR occurred during test\n");
	}
}

void test_sync_cmd(int argc, char **argv) {
	int pid_test = command_spawn_process("test_sync", test_sync_main, argc, argv, 1);

	if (!validate_create_process_error("test_sync", pid_test)) {
		return;
	}

	command_handle_child_process(pid_test, "test_sync");
}
