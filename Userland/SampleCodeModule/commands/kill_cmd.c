#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include <stddef.h>

static void kill_main(int argc, char **argv) {
	int pid = validate_pid_arg("kill", argc, argv, 1);
	if (pid <= 0) {
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
	if (!validate_create_process_error("kill", pid_kill)) {
		return;
	}

	waitpid(pid_kill);
}
