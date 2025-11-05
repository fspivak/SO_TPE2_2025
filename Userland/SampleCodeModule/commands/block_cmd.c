#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include <stddef.h>

static void block_main(int argc, char **argv) {
	int pid = validate_pid_arg("block", argc, argv, 1);
	if (pid <= 0) {
		return;
	}

	int result = block(pid);
	if (result < 0) {
		print("ERROR: Failed to block process ");
		printBase(pid, 10);
		print("\n");
	}
	else {
		print("Process ");
		printBase(pid, 10);
		print(" blocked successfully\n");
	}
}

void block_cmd(int argc, char **argv) {
	int pid_block = create_process("block", block_main, argc, argv, 1);
	if (!validate_create_process_error("block", pid_block)) {
		return;
	}

	waitpid(pid_block);
}
