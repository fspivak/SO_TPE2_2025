#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>

static void loop_main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	while (1) {
		yield();
	}
}

void loop_cmd(int argc, char **argv) {
	int pid_loop = create_process("loop", loop_main, 0, NULL, 4);
	if (pid_loop < 0) {
		print("ERROR: Failed to create process loop\n");
		return;
	}

	print("Loop process started (PID: ");
	printBase(pid_loop, 10);
	print(")\n");
}
