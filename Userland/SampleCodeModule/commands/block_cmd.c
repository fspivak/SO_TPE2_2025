#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

static void block_main(int argc, char **argv) {
	if (argc < 2 || argv == NULL || argv[1] == NULL) {
		print("Usage: block <pid>\n");
		return;
	}

	int pid = satoi(argv[1]);
	if (pid <= 0) {
		print("ERROR: Invalid PID\n");
		return;
	}

	ProcessInfo processes[64];
	int count = ps(processes, 64);
	int found = 0;
	for (int i = 0; i < count; i++) {
		if (processes[i].pid == pid) {
			found = 1;
			break;
		}
	}

	if (!found) {
		print("block: ");
		printBase(pid, 10);
		print(": arguments must be process or job IDs\n");
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
	if (pid_block < 0) {
		print("ERROR: Failed to create process block\n");
		return;
	}

	waitpid(pid_block);
}
