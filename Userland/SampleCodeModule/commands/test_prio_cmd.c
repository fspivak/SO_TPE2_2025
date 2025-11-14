#include "../include/commands.h"
#include "../include/stinUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>
#include <stdint.h>

extern uint64_t test_prio(uint64_t argc, char *argv[]);

void test_prio_main(int argc, char **argv) {
	char *arg_value = "1000000";

	// Si el usuario pasó un argumento, validarlo y usarlo
	if (argc >= 2 && argv != NULL && argv[1] != NULL) {
		int max_value = validate_non_negative_int("test_prio", "max_value", argc, argv, 1);
		if (max_value < 0) {
			return;
		}
		arg_value = argv[1];
	}
	else {
		print_format("test_prio: no max_value provided, using default = %s\n", "1000000");
	}

	print_format("=== Running Priority Test ===\n");
	print_format("Starting test_prio with max_value: %s\n\n", arg_value);

	char *args[] = {arg_value, NULL};

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
	int pid_test = command_spawn_process("test_prio", test_prio_main, argc, argv, 1, NULL);

	if (!validate_create_process_error("test_prio", pid_test)) {
		return;
	}

	command_handle_child_process(pid_test, "test_prio");
}
