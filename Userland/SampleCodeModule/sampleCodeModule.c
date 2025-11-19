// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/* sampleCodeModule.c */
#include "include/libasmUser.h"
#include "include/stinUser.h"
#include "include/terminal.h"
#include <stddef.h>

/* Punto de entrada de userland */
int main() {
	char *argv[] = {NULL};

	process_io_config_t shell_io = {.stdin_type = PROCESS_IO_STDIN_KEYBOARD,
									.stdin_resource = PROCESS_IO_RESOURCE_INVALID,
									.stdout_type = PROCESS_IO_STDOUT_SCREEN,
									.stdout_resource = PROCESS_IO_RESOURCE_INVALID,
									.stderr_type = PROCESS_IO_STDERR_SCREEN,
									.stderr_resource = PROCESS_IO_RESOURCE_INVALID};

	int pid = create_process_with_io("shell", (void *) terminal, 0, argv, 128, &shell_io);
	if (pid < 0) {
		print_format("Error: could not create shell process\n");
		return 1;
	}
	waitpid(pid);
	return 0;
}