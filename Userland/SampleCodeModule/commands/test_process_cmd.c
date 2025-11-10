#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>
#include <stdint.h>

extern uint64_t test_processes(uint64_t argc, char *argv[]);

void test_process_main(int argc, char **argv) {
	char *process_count = "3"; // Por defecto 3 procesos
	if (argc > 1 && argv != NULL && argv[1] != NULL) {
		int count = validate_non_negative_int("test_process", "process_count", argc, argv, 1);
		if (count < 0) {
			return;
		}
		if (count > 64) {
			print_format("ERROR: process_count must be between 1 and 64\n");
			return;
		}
		if (count == 0) {
			print_format("ERROR: process_count must be at least 1\n");
			return;
		}
		process_count = argv[1];
	}

	print_format("=== Running Process Test ===\n");
	print_format("Starting test_process with %s processes...\n\n", process_count);

	char *args[] = {process_count, NULL};

	int64_t result = test_processes(1, args);

	if (result != 0) {
		print_format("test_process: ERROR occurred during test\n");
	}
	else {
		print_format("test_process: Test completed successfully\n");
	}

	print_format("\n=== Process Test Completed ===\n\n");
}

void test_process_cmd(int argc, char **argv) {
	int pid_test = command_spawn_process("test_process", test_process_main, argc, argv, 1, NULL);

	if (!validate_create_process_error("test_process", pid_test)) {
		return;
	}

	command_handle_child_process(pid_test, "test_process");
}
