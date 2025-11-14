// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

extern uint64_t test_mm(uint64_t argc, char *argv[]);

void test_mm_main(int argc, char **argv) {
	char *size_arg = "102400";
	if (argc > 1 && argv != NULL && argv[1] != NULL) {
		int size = validate_non_negative_int("test_mm", "size", argc, argv, 1);
		if (size < 0) {
			return;
		}
		size_arg = argv[1];
	}

	print_format("=== Running Memory Manager Test ===\n");
	print_format("Running test_mm with size: %s bytes\n\n", size_arg);

	char *args[] = {size_arg, NULL};

	int64_t result = test_mm(1, args);

	if (result != 0) {
		print_format("test_mm: ERROR occurred during test\n");
	}
	else {
		print_format("test_mm: Test completed successfully\n");
	}

	print_format("\n=== Memory Manager Test Completed ===\n\n");
}

void test_mm_cmd(int argc, char **argv) {
	int pid_test = command_spawn_process("test_mm", test_mm_main, argc, argv, 128, NULL);
	if (!validate_create_process_error("test_mm", pid_test)) {
		return;
	}

	command_handle_child_process(pid_test, "test_mm");
}
