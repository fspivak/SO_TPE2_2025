#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

extern uint64_t test_mm(uint64_t argc, char *argv[]);

void test_mm_main(int argc, char **argv) {
	// Usar argumento por defecto si no se proporciona
	char *size_arg = "1048576"; // 1MB por defecto
	if (argc > 0 && argv[0] != NULL) {
		size_arg = argv[0];
	}

	print("=== Running Memory Manager Test ===\n");
	print("Running test_mm with size: ");
	print(size_arg);
	print(" bytes\n\n");

	char *args[] = {size_arg, NULL};

	int64_t result = test_mm(1, args);

	if (result != 0) {
		print("test_mm: ERROR occurred during test\n");
	}
	else {
		print("test_mm: Test completed successfully\n");
	}

	print("\n=== Memory Manager Test Completed ===\n\n");
}

void test_mm_cmd(int argc, char **argv) {
	int pid_test = create_process("test_mm", test_mm_main, argc, argv, 4);
	if (pid_test < 0) {
		print("ERROR: Failed to create process test_mm\n");
		return;
	}

	waitpid(pid_test);
}
