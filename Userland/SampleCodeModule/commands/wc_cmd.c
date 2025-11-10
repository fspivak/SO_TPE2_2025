#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

static void wc_print_result(int lines, int words, int chars) {
	print_format("\nLines: %d Words: %d Chars: %d\n", lines, words, chars);
}

void wc_cmd(int argc, char **argv) {
	(void) argc;
	(void) argv;

	char c;
	int lines = 0, words = 0, chars = 0;
	int in_word = 0;
	int bytes_read;

	while ((bytes_read = read_bytes(0, &c, 1)) > 0) {
		putchar(c);
		chars += bytes_read;

		if (c == '\n') {
			lines++;
			in_word = 0;
		}
		else if (c == ' ' || c == '\t') {
			in_word = 0;
		}
		else if (!in_word) {
			in_word = 1;
			words++;
		}
	}

	wc_print_result(lines, words, chars);
}

void wc_pipe_main(int argc, char **argv) {
	if (argc < 3 || argv == NULL) {
		print_format("wc pipe: invalid arguments\n");
		return;
	}

	int pipe_id = (int) satoi(argv[argc - 2]);
	const char *mode = argv[argc - 1];

	if (pipe_id < 0 || mode == NULL || strcmp(mode, "read") != 0) {
		print_format("wc pipe: invalid pipe mode\n");
		return;
	}

	char c;
	int lines = 0, words = 0, chars = 0;
	int in_word = 0;
	int bytes_read;

	while ((bytes_read = (int) my_pipe_read((uint64_t) pipe_id, &c, 1)) > 0) {
		chars += bytes_read;

		if (c == '\n') {
			lines++;
			in_word = 0;
		}
		else if (c == ' ' || c == '\t') {
			in_word = 0;
		}
		else if (!in_word) {
			in_word = 1;
			words++;
		}
	}

	my_pipe_close((uint64_t) pipe_id);
	wc_print_result(lines, words, chars);
}
