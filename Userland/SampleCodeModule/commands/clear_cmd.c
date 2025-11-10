#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/screen.h"
#include "../include/stinUser.h"
#include <stdint.h>

static void clear_main(int argc, char **argv) {
	(void) argc; // No usamos (convencion)
	(void) argv; // No usamos (convencion)

	clearScreen();
}

void clear_cmd(int argc, char **argv) {
	int pid_clear = create_process("clear", clear_main, argc, argv, 1);
	if (pid_clear < 0) {
		print_format("ERROR: Failed to create process clear\n");
		return;
	}

	waitpid(pid_clear);
}