// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include <stddef.h>

void nice_cmd(int argc, char **argv) {
	int pid = validate_pid_arg("nice", argc, argv, 1);
	if (pid <= 0) {
		return;
	}

	int priority = validate_priority_arg("nice", argc, argv, 2);
	if (priority < 0) {
		return;
	}

	int result = nice(pid, priority);
	if (result < 0) {
		print_format("ERROR: Failed to change priority of process %d\n", pid);
	}
	else {
		print_format("Priority of process %d changed to %d\n", pid, priority);
	}
}
