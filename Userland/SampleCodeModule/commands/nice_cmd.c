#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

static void nice_main(int argc, char **argv) {
	if (argc < 3 || argv == NULL || argv[1] == NULL || argv[2] == NULL) {
		print("Usage: nice <pid> <priority>\n");
		print("Priority range: 0-255 (default: 4)\n");
		return;
	}

	int pid = satoi(argv[1]);
	int priority = satoi(argv[2]);

	if (pid <= 0) {
		print("ERROR: Invalid PID\n");
		return;
	}

	if (priority < 0 || priority > 255) {
		print("ERROR: Priority must be between 0 and 255\n");
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
		print("nice: ");
		printBase(pid, 10);
		print(": arguments must be process or job IDs\n");
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
	if (pid_nice < 0) {
		print("ERROR: Failed to create process nice\n");
		return;
	}

	waitpid(pid_nice);
}
