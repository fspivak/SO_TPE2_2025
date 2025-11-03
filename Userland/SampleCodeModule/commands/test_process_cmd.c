#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>
#include <stdint.h>

extern uint64_t test_processes(uint64_t argc, char *argv[]);

void test_process_main(int argc, char **argv) {
	// Usar argumento por defecto si no se proporciona
	char *process_count = "3";
	if (argc > 1 && argv != NULL && argv[1] != NULL) {
		process_count = argv[1];
	}

	print("=== Running Process Test ===\n");
	print("Starting test_process with ");
	print(process_count);
	print(" processes...\n\n");

	char *args[] = {process_count, NULL};

	int64_t result = test_processes(1, args);

	if (result != 0) {
		print("test_process: ERROR occurred during test\n");
	}
	else {
		print("test_process: Test completed successfully\n");
	}

	print("\n=== Process Test Completed ===\n\n");
}

void test_process_cmd(int argc, char **argv) {
	int pid_test = my_create_process("test_process", test_process_main, argc, argv);

	if (pid_test < 0) {
		print("ERROR: Failed to create process test_process\n");
		return;
	}

	waitpid(pid_test);
}
