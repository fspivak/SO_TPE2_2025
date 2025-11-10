#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"

void cat_main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	char c;

	print_format("Entering cat (Ctrl+D for EOF)\n");

	while (1) {
		c = getchar();

		if (c == 0) {
			continue;
		}

		if (c == -1) {
			print_format("\n[EOF]\n");
			return;
		}

		putchar(c);
	}
}

void cat_cmd(int argc, char **argv) {
	int pid_cat = command_spawn_process("cat", cat_main, argc, argv, 1);
	if (!validate_create_process_error("cat", pid_cat)) {
		return;
	}

	command_handle_child_process(pid_cat, "cat");
}
