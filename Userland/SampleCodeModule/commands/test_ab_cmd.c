#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

extern int64_t test_ab(uint64_t argc, char *argv[]);

void test_ab_main(int argc, char **argv) {
	(void) argc; // No usamos (convencion)
	(void) argv; // No usamos (convencion)

	print_format("=== Running AB Test ===\n");
	print_format("Starting test_ab (two processes printing A and B)...\n\n");

	int64_t result = test_ab(0, NULL);

	if (result != 0) {
		print_format("test_ab: ERROR occurred during test\n");
	}
	else {
		print_format("test_ab: Test completed successfully\n");
	}

	print_format("\n=== AB Test Completed ===\n\n");
}

void test_ab_cmd(int argc, char **argv) {
	int pid_test = create_process("test_ab", test_ab_main, argc, argv, 128);
	if (pid_test < 0) {
		print_format("ERROR: Failed to create process test_ab\n");
		return;
	}

	waitpid(pid_test);
}
