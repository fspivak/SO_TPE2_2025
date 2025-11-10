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
	int pid_loop = create_process("loop", loop_main, 0, NULL, 200);
	if (pid_loop < 0) {
		print_format("ERROR: Failed to create process loop\n");
		return;
	}

	print_format("Loop process started (PID: %d)\n", pid_loop);
}
