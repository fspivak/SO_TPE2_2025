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
	exit(); /* Terminate process using syscall */
}

/* Main test: creates both processes and waits */
uint64_t test_ab(uint64_t argc, char *argv[]) {
	(void) argc;
	(void) argv;

	print("Starting AB test...\n");
	print("This will create two processes that print 'A' and 'B' alternately.\n");
	print("You should see them switching back and forth.\n\n");

	int pid_a = create_process("process_a", process_a, 0, NULL, 200);
	if (pid_a < 0) {
		print("ERROR: Failed to create process A\n");
		return -1;
	}

	int pid_b = create_process("process_b", process_b, 0, NULL, 200);
	if (pid_b < 0) {
		print("ERROR: Failed to create process B\n");
		return -1;
	}

	// Esperamos a que los procesos terminen
	waitpid(pid_a);
	waitpid(pid_b);

	print("\n\n=== AB Test Completed ===\n");
	print("Both processes have finished.\n");

	return 0;
}
