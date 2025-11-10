#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include <stddef.h>

void block_cmd(int argc, char **argv) {
	int pid = validate_pid_arg("block", argc, argv, 1);
	if (pid <= 0) {
		return;
	}

	int result = block(pid);
	if (result < 0) {
		print_format("ERROR: Failed to block process %d\n", pid);
	}
	else {
		print_format("Process %d blocked successfully\n", pid);
	}
}
