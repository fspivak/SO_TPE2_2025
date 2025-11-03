#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../include/terminal.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

extern int64_t test_processes(uint64_t argc, char *argv[]);

extern int64_t test_ab(uint64_t argc, char *argv[]);

extern uint64_t test_mm(uint64_t argc, char *argv[]);

void test_ab_entry(uint64_t argc, char *argv[]) {
	print("\n=== Running AB Test ===\n");

	uint64_t result = test_ab(argc, argv);

	if (result != 0) {
		print("test_ab: ERROR occurred during test\n");
	}
	else {
		print("test_ab: Test completed successfully\n");
	}

	print("\n=== AB Test Completed ===\n\n");
	return;
}

void test_process_entry(uint64_t argc, char *argv[]) {
	print("\n=== Running Process Test ===\n");

	int process_count = 3; // TODO:revisar
	if (argc > 0 && argv != NULL && argv[0] != NULL && argv[0][0] != '\0') {
		process_count = satoi(argv[0]);
		if (process_count <= 0 || process_count > 64) {
			print("Invalid process count (1-64). Using default: 3\n");
			process_count = 3;
		}
	}

	print("Starting test_process with ");
	printBase(process_count, 10);
	print(" processes...\n\n");

	char process_str[10];
	intToString(process_count, process_str);
	char *new_argv[] = {process_str};

	uint64_t result = test_processes(1, new_argv);

	if (result == -1)
		print("test_process: ERROR occurred during test\n");
	else
		print("test_process: Test completed successfully\n");

	print("\n=== Process Test Completed ===\n\n");
	return;
}

void clock_entry(uint64_t argc, char *argv[]) {
	char str[9];
	getClock(str);	 // obtiene la hora del kernel
	printClock(str); // imprime la hora
	print("\n");
	return;
}

void test_mm_entry(uint64_t argc, char *argv[]) {
	print("\n=== Running Memory Manager Test ===\n");

	extern uint64_t test_mm(uint64_t argc, char *argv[]);

	char *mm_argv[] = {NULL};
	if (argc > 0 && argv != NULL && argv[0] != NULL && argv[0][0] != '\0') {
		print("Running test_mm with custom size: ");
		print(argv[0]);
		print(" bytes\n");
		mm_argv[0] = argv[0];
	}
	else {
		print("Running test_mm with default size: 1MB\n");
		mm_argv[0] = "1048576"; /* 1MB default */
	}

	uint64_t result = test_mm(1, mm_argv);

	if (result == -1) {
		print("test_mm: ERROR occurred during test\n");
	}
	else {
		print("test_mm: Test completed successfully\n");
	}

	print("\n=== Memory Manager Test Completed ===\n\n");
	return;
}
