// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/stinUser.h"
#include "include/syscall.h"
#include "include/test_util.h"
#include <stdint.h>

#define TOTAL_PROCESSES 3

#define LOWEST 0
#define MEDIUM 128
#define HIGHEST 255

int64_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};

uint64_t max_value = 0;

static void zero_to_max_wrapper(int argc, char **argv) {
	(void) argc;
	(void) argv;
	uint64_t value = 0;

	while (value++ != max_value)
		;

	print_format("PROCESS %d DONE!\n", (int) my_getpid());
}

uint64_t test_prio(uint64_t argc, char *argv[]) {
	int64_t pids[TOTAL_PROCESSES];
	char *ztm_argv[] = {0};
	uint64_t i;

	if (argc != 1)
		return -1;

	if ((max_value = satoi(argv[0])) <= 0)
		return -1;

	print_format("SAME PRIORITY...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++)
		pids[i] = my_create_process("zero_to_max", zero_to_max_wrapper, 0, ztm_argv);

	// Expect to see them finish at the same time

	for (i = 0; i < TOTAL_PROCESSES; i++)
		my_wait(pids[i]);

	print_format("SAME PRIORITY, THEN CHANGE IT...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++) {
		pids[i] = my_create_process("zero_to_max", zero_to_max_wrapper, 0, ztm_argv);
		my_nice(pids[i], prio[i]);
		print_format("  PROCESS %d NEW PRIORITY: %d\n", (int) pids[i], (int) prio[i]);
	}

	// Expect the priorities to take effect

	for (i = 0; i < TOTAL_PROCESSES; i++)
		my_wait(pids[i]);

	print_format("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

	for (i = 0; i < TOTAL_PROCESSES; i++) {
		pids[i] = my_create_process("zero_to_max", zero_to_max_wrapper, 0, ztm_argv);
		my_block(pids[i]);
		my_nice(pids[i], prio[i]);
		print_format("  PROCESS %d NEW PRIORITY: %d\n", (int) pids[i], (int) prio[i]);
	}

	for (i = 0; i < TOTAL_PROCESSES; i++)
		my_unblock(pids[i]);

	// Expect the priorities to take effect

	for (i = 0; i < TOTAL_PROCESSES; i++)
		my_wait(pids[i]);

	return 0;
}
