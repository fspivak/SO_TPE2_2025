#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

static void kill_main(int argc, char **argv) {
	if (argc < 2 || argv == NULL || argv[1] == NULL) {
		print("Usage: kill <pid>\n");
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
		print("kill: ");
		printBase(pid, 10);
		print(": arguments must be process or job IDs\n");
		return;
	}

	if (kill(pid) < 0) {
		print("ERROR: Failed to kill process ");
		printBase(pid, 10);
		print("\n");
	}
}

void kill_cmd(int argc, char **argv) {
	int pid_kill = create_process("kill", kill_main, argc, argv, 0);
	if (pid_kill < 0) {
		print("ERROR: Failed to create process kill\n");
		return;
	}

	waitpid(pid_kill);
}
