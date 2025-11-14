// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/screen.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

static void clear_main(int argc, char **argv) {
	(void) argc; // No usamos (convencion)
	(void) argv; // No usamos (convencion)

	clearScreen();
}

void clear_cmd(int argc, char **argv) {
	int pid_clear = command_spawn_process("clear", clear_main, argc, argv, 1, NULL);
	if (pid_clear < 0) {
		print_format("ERROR: Failed to create process clear\n");
		return;
	}

	command_handle_child_process(pid_clear, "clear");
}