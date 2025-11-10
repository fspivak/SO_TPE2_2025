#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

static int validate_pid_exists(int pid) {
	ProcessInfo *processes = (ProcessInfo *) malloc(sizeof(ProcessInfo) * 64);
	if (processes == NULL) {
		print_format("ERROR: Unable to list processes\n");
		return 0;
	}

	int count = ps(processes, 64);
	if (count <= 0) {
		print_format("ERROR: Unable to list processes\n");
		free(processes);
		return 0;
	}

	int found = 0;
	for (int i = 0; i < count; i++) {
		if (processes[i].pid == pid) {
			found = 1;
			break;
		}
	}

	free(processes);
	return found;
}

int validate_pid_arg(const char *cmd_name, int argc, char **argv, int arg_index) {
	if (argc <= arg_index || argv == NULL || argv[arg_index] == NULL) {
		if (strcmp(cmd_name, "nice") == 0) {
			print_format("%s: missing operand\nUsage: %s <pid> <priority>\nPriority range: 0-255\n", cmd_name,
						 cmd_name);
		}
		else {
			print_format("%s: missing operand\nUsage: %s <pid>\n", cmd_name, cmd_name);
		}
		return 0;
	}

	int pid = satoi(argv[arg_index]);
	if (pid <= 0) {
		print_format("ERROR: Invalid PID (must be a positive number)\n");
		return 0;
	}

	if (!validate_pid_exists(pid)) {
		print_format("%s: %d: arguments must be process or job IDs\n", cmd_name, pid);
		return 0;
	}

	return pid;
}

int validate_priority_arg(const char *cmd_name, int argc, char **argv, int arg_index) {
	if (argc <= arg_index || argv == NULL || argv[arg_index] == NULL) {
		print_format("%s: missing priority operand\nUsage: %s <pid> <priority>\nPriority range: 0-255\n", cmd_name,
					 cmd_name);
		return -1;
	}

	int priority = satoi(argv[arg_index]);
	if (priority < 0 || priority > 255) {
		print_format("ERROR: Priority must be between 0 and 255\n");
		return -1;
	}

	return priority;
}

int validate_non_negative_int(const char *cmd_name, const char *arg_name, int argc, char **argv, int arg_index) {
	if (argc <= arg_index || argv == NULL || argv[arg_index] == NULL) {
		print_format("%s: missing %s operand\n", cmd_name, arg_name);
		return -1;
	}

	int value = satoi(argv[arg_index]);
	if (value < 0) {
		print_format("ERROR: %s must be a non-negative number\n", arg_name);
		return -1;
	}

	return value;
}

int validate_create_process_error(const char *cmd_name, int pid) {
	if (pid < 0) {
		print_format("ERROR: Failed to create process '%s': insufficient memory or process table full\n", cmd_name);
		return 0;
	}
	return 1;
}
