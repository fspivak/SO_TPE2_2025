#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

static void cat_run_interactive(void) {
	char c;
	char *start_msg = "Entering cat (Ctrl+D for EOF)\n";
	write(2, start_msg, 31, 0x00ffffff, 0);

	while (read_bytes(0, &c, 1) > 0) {
		putchar(c);
	}

	char *eof_msg = "\n[EOF]\n";
	write(2, eof_msg, 7, 0x00ffffff, 0);
}

void cat_main(int argc, char **argv) {
	(void) argc;
	(void) argv;
	cat_run_interactive();
}

void cat_pipe_main(int argc, char **argv) {
	if (argc < 3 || argv == NULL) {
		print_format("cat pipe: invalid arguments\n");
		return;
	}

	int pipe_id = (int) satoi(argv[argc - 2]);
	const char *mode = argv[argc - 1];

	if (pipe_id < 0 || mode == NULL || strcmp(mode, "write") != 0) {
		print_format("cat pipe: invalid pipe mode\n");
		return;
	}

	char c;
	while (read_bytes(0, &c, 1) > 0) {
		if (my_pipe_write((uint64_t) pipe_id, &c, 1) < 0) {
			print_format("cat pipe: write error\n");
			break;
		}
		write(2, &c, 1, 0x00ffffff, 0);
	}

	my_pipe_close((uint64_t) pipe_id);
}

void cat_cmd(int argc, char **argv) {
	int pid_cat = command_spawn_process("cat", cat_main, argc, argv, 1, NULL);
	if (!validate_create_process_error("cat", pid_cat)) {
		return;
	}

	command_handle_child_process(pid_cat, "cat");
}
