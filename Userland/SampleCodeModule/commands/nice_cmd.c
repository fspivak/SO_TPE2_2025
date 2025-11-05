#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include <stddef.h>

static void nice_main(int argc, char **argv) {
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
		print("ERROR: Failed to change priority of process ");
		printBase(pid, 10);
		print("\n");
	}
	else {
		print("Priority of process ");
		printBase(pid, 10);
		print(" changed to ");
		printBase(priority, 10);
		print("\n");
	}
}

void nice_cmd(int argc, char **argv) {
	int pid_nice = create_process("nice", nice_main, argc, argv, 1);
	if (!validate_create_process_error("nice", pid_nice)) {
		return;
	}

	waitpid(pid_nice);
}
