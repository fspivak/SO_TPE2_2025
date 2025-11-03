#include "../include/stinUser.h"
#include "include/syscall.h"
#include "include/test_util.h"
#include <stdint.h>

#define TOTAL_PROCESSES 3

#define LOWEST 0  // TODO: Change as required
#define MEDIUM 1  // TODO: Change as required
#define HIGHEST 2 // TODO: Change as required

int64_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};

uint64_t max_value = 0;

void zero_to_max() {
	uint64_t value = 0;

	while (value++ != max_value)
		;

	print("PROCESS ");
	printBase(my_getpid(), 10);
	print(" DONE!\n");
}

uint64_t test_prio(uint64_t argc, char *argv[]) {
	int64_t pids[TOTAL_PROCESSES];
	char *ztm_argv[] = {0};
	uint64_t i;

	if (argc != 1)
		return -1;

	if ((max_value = satoi(argv[0])) <= 0)
		return -1;

	print("SAME PRIORITY...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++)
		pids[i] = my_create_process("zero_to_max", zero_to_max, 0, ztm_argv);

	// Expect to see them finish at the same time

	for (i = 0; i < TOTAL_PROCESSES; i++)
		my_wait(pids[i]);

	print("SAME PRIORITY, THEN CHANGE IT...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++) {
		pids[i] = my_create_process("zero_to_max", zero_to_max, 0, ztm_argv);
		my_nice(pids[i], prio[i]);
		print("  PROCESS ");
		printBase(pids[i], 10);
		print(" NEW PRIORITY: ");
		printBase(prio[i], 10);
		print("\n");
	}

	// Expect the priorities to take effect

	for (i = 0; i < TOTAL_PROCESSES; i++)
		my_wait(pids[i]);

	print("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++) {
		pids[i] = my_create_process("zero_to_max", zero_to_max, 0, ztm_argv);
		my_block(pids[i]);
		my_nice(pids[i], prio[i]);
		print("  PROCESS ");
		printBase(pids[i], 10);
		print(" NEW PRIORITY: ");
		printBase(prio[i], 10);
		print("\n");
	}

	for (i = 0; i < TOTAL_PROCESSES; i++)
		my_unblock(pids[i]);

	// Expect the priorities to take effect

	for (i = 0; i < TOTAL_PROCESSES; i++)
		my_wait(pids[i]);

	return 0;
}
