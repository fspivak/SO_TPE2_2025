#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include <stddef.h>

void kill_cmd(int argc, char **argv) {
	int pid = validate_pid_arg("kill", argc, argv, 1);
	if (pid <= 0) {
		return;
	}

	if (kill(pid) < 0) {
		print_format("ERROR: Failed to kill process %d\n", pid);
	}
}
