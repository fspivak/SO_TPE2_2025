#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

void wc_cmd(int argc, char **argv) {
	int is_write = 0, is_read = 0, pipe_id = -1;

	for (int i = 0; i < argc; i++) {
		if (!argv[i])
			continue;
		if (strcmp(argv[i], "read") == 0 && i > 0) {
			is_read = 1;
			pipe_id = satoi(argv[i - 1]);
		}
		else if (strcmp(argv[i], "write") == 0 && i > 0) {
			is_write = 1;
			pipe_id = satoi(argv[i - 1]);
		}
	}

	char c;
	int lines = 0, words = 0, chars = 0;
	int in_word = 0;

	if (is_write) {
		print_format("Enter text (Ctrl+D to finish):\n");
		while ((c = getchar()) != -1) {
			// eco local para que el usuario vea lo que tipea
			putchar(c);
			int w = my_pipe_write(pipe_id, &c, 1);
			if (w <= 0)
				print_format("[DEBUG] wc(write): write error\n");
		}
		my_pipe_close(pipe_id);
		print_format("[EOF]\n");
		return;
	}

	if (is_read) {
		print_format("[DEBUG] wc: leyendo desde pipe...\n");
		int total = 0;
		while (my_pipe_read(pipe_id, &c, 1) > 0) {
			total++;
			chars++;
			if (c == '\n')
				lines++;
			if (c == ' ' || c == '\n' || c == '\t')
				in_word = 0;
			else if (!in_word) {
				in_word = 1;
				words++;
			}
		}
		print_format("[DEBUG] wc: EOF (read %d bytes)\n", total);
	}
	else {
		print_format("Enter text (Ctrl+D to finish):\n");
		while ((c = getchar()) != -1) {
			putchar(c);
			chars++;
			if (c == '\n') {
				lines++;
			}
			if (c == ' ' || c == '\n' || c == '\t') {
				in_word = 0;
			}
			else if (!in_word) {
				in_word = 1;
				words++;
			}
		}
	}

	print_format("\n Lines: %d Words: %d Chars: %d\n", lines, words, chars);
}
