/* sampleCodeModule.c */
#include "include/libasmUser.h"
#include "include/stinUser.h"
#include "include/terminal.h"
#include <stddef.h>

/* Punto de entrada de userland */
int main() {
	char *argv[] = {NULL};
	int pid = create_process("shell", (void *) terminal, 0, argv, 8);
	if (pid < 0) {
		print("Error: could not create clock process\n");
		return 1;
	}
	waitpid(pid);
	return 0;
}