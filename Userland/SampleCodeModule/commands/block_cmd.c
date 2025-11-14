// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

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
		return;
	}

	if (result == 1) {
		print_format("Process %d blocked successfully\n", pid);
		return;
	}

	if (result == 2) {
		int unblock_result = unblock(pid);
		if (unblock_result < 0) {
			print_format("ERROR: Failed to unblock process %d\n", pid);
		}
		else {
			print_format("Process %d unblocked successfully\n", pid);
		}
	}
}
