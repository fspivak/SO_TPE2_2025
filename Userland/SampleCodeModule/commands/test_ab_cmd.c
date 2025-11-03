#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

extern int64_t test_ab(uint64_t argc, char *argv[]);

void test_ab_main(int argc, char **argv) {
	(void) argc; // No usamos (convencion)
	(void) argv; // No usamos (convencion)

	print("=== Running AB Test ===\n");
	print("Starting test_ab (two processes printing A and B)...\n\n");

	int64_t result = test_ab(0, NULL);

	if (result != 0) {
		print("test_ab: ERROR occurred during test\n");
	}
	else {
		print("test_ab: Test completed successfully\n");
	}

	print("\n=== AB Test Completed ===\n\n");
}

void test_ab_cmd(int argc, char **argv) {
	int pid_test = create_process("test_ab", test_ab_main, argc, argv, 4);
	if (pid_test < 0) {
		print("ERROR: Failed to create process test_ab\n");
		return;
	}

	waitpid(pid_test);
}
