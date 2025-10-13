#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

/* Declaracion de syscalls */
int create_process(const char *name, void (*entry)(int, char **), int argc, char **argv, int priority);
void yield(void);
void exit(void);

/* Process that prints 'A' a limited number of times */
void process_a(int argc, char **argv) {
	(void) argc;
	(void) argv;
	for (int i = 0; i < 50; i++) {
		print("A");
		yield(); /* Force context switch */
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
		yield(); /* Force context switch */
	}
	print("\n[Process B finished]\n");
	exit(); /* Terminate process using syscall */
}

/* Main test: creates both processes and waits */
int64_t test_ab(uint64_t argc, char *argv[]) {
	(void) argc;
	(void) argv;

	print("Starting AB test...\n");
	print("Creating process A...\n");

	int pid_a = create_process("process_a", process_a, 0, NULL, 128);
	if (pid_a < 0) {
		print("ERROR: Failed to create process A\n");
		return -1;
	}

	print("Creating process B...\n");
	int pid_b = create_process("process_b", process_b, 0, NULL, 128);
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
