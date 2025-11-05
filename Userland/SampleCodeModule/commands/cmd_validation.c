#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

static int validate_pid_exists(int pid) {
	ProcessInfo processes[64];
	int count = ps(processes, 64);
	for (int i = 0; i < count; i++) {
		if (processes[i].pid == pid) {
			return 1;
		}
	}
	return 0;
}

int validate_pid_arg(const char *cmd_name, int argc, char **argv, int arg_index) {
	if (argc <= arg_index || argv == NULL || argv[arg_index] == NULL) {
		print((char *) cmd_name);
		print(": missing operand\n");
		print("Usage: ");
		print((char *) cmd_name);
		print(" <pid>\n");
		return 0;
	}

	int pid = satoi(argv[arg_index]);
	if (pid <= 0) {
		print("ERROR: Invalid PID (must be a positive number)\n");
		return 0;
	}

	if (!validate_pid_exists(pid)) {
		print((char *) cmd_name);
		print(": ");
		printBase(pid, 10);
		print(": arguments must be process or job IDs\n");
		return 0;
	}

	return pid;
}

int validate_priority_arg(const char *cmd_name, int argc, char **argv, int arg_index) {
	if (argc <= arg_index || argv == NULL || argv[arg_index] == NULL) {
		print((char *) cmd_name);
		print(": missing priority operand\n");
		print("Usage: ");
		print((char *) cmd_name);
		print(" <pid> <priority>\n");
		print("Priority range: 0-255\n");
		return -1;
	}

	int priority = satoi(argv[arg_index]);
	if (priority < 0 || priority > 255) {
		print("ERROR: Priority must be between 0 and 255\n");
		return -1;
	}

	return priority;
}

int validate_non_negative_int(const char *cmd_name, const char *arg_name, int argc, char **argv, int arg_index) {
	if (argc <= arg_index || argv == NULL || argv[arg_index] == NULL) {
		print((char *) cmd_name);
		print(": missing ");
		print((char *) arg_name);
		print(" operand\n");
		return -1;
	}

	int value = satoi(argv[arg_index]);
	if (value < 0) {
		print("ERROR: ");
		print((char *) arg_name);
		print(" must be a non-negative number\n");
		return -1;
	}

	return value;
}

int validate_create_process_error(const char *cmd_name, int pid) {
	if (pid < 0) {
		print("ERROR: Failed to create process '");
		print((char *) cmd_name);
		print("': insufficient memory or process table full\n");
		return 0;
	}
	return 1;
}
