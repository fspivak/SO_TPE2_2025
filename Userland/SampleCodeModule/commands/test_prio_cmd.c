#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>
#include <stdint.h>

extern uint64_t test_prio(uint64_t argc, char *argv[]);

void test_prio_main(int argc, char **argv) {
	if (argc < 2 || argv == NULL || argv[1] == NULL) {
		print_format("test_prio: Usage: test_prio <max_value>\n");
		print_format("  max_value: Maximum value for the loop (must be positive)\n");
		print_format("  Example: test_prio 1000000\n");
		return;
	}

	int max_value = validate_non_negative_int("test_prio", "max_value", argc, argv, 1);
	if (max_value < 0) {
		return;
	}
	if (max_value == 0) {
		print_format("ERROR: max_value must be greater than 0\n");
		return;
	}

	print_format("=== Running Priority Test ===\n");
	print_format("Starting test_prio with max_value: %s\n\n", argv[1]);

	char *args[] = {argv[1], NULL};

	int64_t result = test_prio(1, args);

	if (result != 0) {
		print_format("test_prio: ERROR occurred during test\n");
	}
	else {
		print_format("test_prio: Test completed successfully\n");
	}

	print_format("\n=== Priority Test Completed ===\n\n");
}

void test_prio_cmd(int argc, char **argv) {
	int pid_test = my_create_process("test_prio", test_prio_main, argc, argv);

	if (!validate_create_process_error("test_prio", pid_test)) {
		return;
	}

	waitpid(pid_test);
}
