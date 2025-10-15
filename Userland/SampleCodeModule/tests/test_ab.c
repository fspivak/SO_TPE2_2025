#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

/* Process that prints 'A' a limited number of times */
void process_a(int argc, char **argv) {
	(void) argc;
	(void) argv;
	for (int i = 0; i < 50; i++) {
		print("A");
		// yield(); /* Force context switch */
	}
	print("\n[Process A finished]\n");
	exit(); /* Terminate process using syscall */
}

/* Process that prints 'B' a limited number of times */
void process_b(int argc, char **argv) {
	(void) argc;
	(void) argv;
	for (int i = 0; i < 50; i++) {
		print("B");
		// yield(); /* Force context switch */
	}
	print("\n[Process B finished]\n");
	exit(); /* Terminate process using syscall */
}

/* Main test: creates both processes and waits */
int64_t test_ab(uint64_t argc, char *argv[]) {
	(void) argc;
	(void) argv;

	print("Starting AB test...\n");
	print("This will create two processes that print 'A' and 'B' alternately.\n");
	print("You should see them switching back and forth.\n\n");
	print("Creating process A...\n");

	int pid_a = create_process("process_a", process_a, 0, NULL, 4);
	if (pid_a < 0) {
		print("ERROR: Failed to create process A\n");
		return -1;
	}

	print("Creating process B...\n");
	int pid_b = create_process("process_b", process_b, 0, NULL, 1);
	if (pid_b < 0) {
		print("ERROR: Failed to create process B\n");
		return -1;
	}

	print("Both processes created! You should see 'A' and 'B' alternating.\n");
	print("Each process will print 50 times and then terminate.\n\n");

	/* Main process yields enough times to let children finish */
	/* Each child does 50 iterations, so we need to yield enough times */
	for (int i = 0; i < 200; i++) {
		yield();
	}

	print("\n=== AB Test Completed ===\n");
	print("Both processes should have finished by now.\n");

	return 0;
}

/* Wrapper function for terminal integration */
void run_test_ab() {
	extern int64_t test_ab(uint64_t argc, char *argv[]);

	char *argv[] = {NULL};
	int64_t result = test_ab(0, argv);

	if (result != 0) {
		print("test_ab: ERROR occurred during test\n");
	}
	else {
		print("test_ab: Test completed successfully\n");
	}
}
