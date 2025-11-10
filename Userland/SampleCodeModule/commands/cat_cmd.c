

// void cat_cmd(int argc, char **argv) {
// 	int pid_cat = create_process("cat", cat_main, argc, argv, 1);
// 	if (!validate_create_process_error("cat", pid_cat))
// 		return;

// 	waitpid(pid_cat);
// }
#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stdint.h>

void cat_cmd(int argc, char **argv) {
	int is_write = 0, is_read = 0, pipe_id = -1;

	for (int i = 0; i < argc; i++) {
		if (!argv[i])
			continue;
		if (strcmp(argv[i], "write") == 0 && i > 0) {
			is_write = 1;
			pipe_id = satoi(argv[i - 1]);
		}
		else if (strcmp(argv[i], "read") == 0 && i > 0) {
			is_read = 1;
			pipe_id = satoi(argv[i - 1]);
		}
	}

	char c;

	if (is_read) {
		print_format("[DEBUG] cat: reading from pipe...\n");
		int total = 0;
		while (my_pipe_read(pipe_id, &c, 1) > 0) {
			putchar(c);
			total++;
		}
		print_format("[DEBUG] cat: EOF (read %d bytes)\n", total);
		return;
	}

	if (is_write) {
		print_format("Entrando a cat (Ctrl+D para EOF)\n");
		while ((c = getchar()) != -1) {
			// eco local opcional para “ver” lo que escribo al pipe
			putchar(c);
			int w = my_pipe_write(pipe_id, &c, 1);
			if (w <= 0)
				print_format("[DEBUG] cat: write error\n");
		}
		my_pipe_close(pipe_id);
		print_format("[EOF]\n");
		return;
	}

	print_format("Entrando a cat (Ctrl+D para EOF)\n");
	// while ((c = getchar()) != -1)
	//     putchar(c);
	// print_format("[EOF]\n");

	while ((c = getchar()) != -1) {
		print_format("[DEBUG] cat got '%c' (%d)\n", c, c);
		my_pipe_write(pipe_id, &c, 1);
	}
}
