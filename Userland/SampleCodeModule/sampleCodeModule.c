/* sampleCodeModule.c */
#include "include/libasmUser.h"
#include "include/stinUser.h"
#include "include/terminal.h"
#include <stddef.h>

/* Punto de entrada de userland */
int main() {
	char *argv[] = {NULL};
	int pid = create_process("shell", (void *) terminal, 0, argv, 128);
	if (pid < 0) {
		print_format("Error: could not create shell process\n");
		return 1;
	}
	waitpid(pid);
	return 0;
}