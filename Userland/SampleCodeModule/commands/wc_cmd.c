#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

void wc_cmd(int argc, char **argv) {
	int pipe_mode = 0;
	int pipe_id = -1;

	// Si se ejecuta en modo pipe: argv[1] = id, argv[2] = "read"
	if (argc >= 3 && strcmp(argv[2], "read") == 0) {
		pipe_mode = 1;
		pipe_id = satoi(argv[1]);
	}

	char c;
	int lines = 1, words = 0, chars = 0;
	int in_word = 0;

	if (pipe_mode) {
		// Leemos del pipe hasta que se vacíe
		while (my_pipe_read(pipe_id, &c, 1) > 0) {
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
	}
	// else {
	// 	// Leemos desde stdin (usando getchar porque read es void)
	// 	while ((c = getchar()) != -1) {
	// 		putchar(c);
	// 		chars++;
	// 		if (c == ' ' || c == '\t')
	// 			in_word = 0;
	// 		else if (!in_word) {
	// 			in_word = 1;
	// 			words++;
	// 		}
	// 	}
	// 	lines = 1; // una sola línea ingresada
	// }
	else {
		// Leemos desde stdin (usando getchar porque read es void)
		while ((c = getchar()) != -1) {
			putchar(c);
			chars++;

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
	}

	print("\n");
	print("Lines: ");
	print_int_padded(lines, 0);
	print("  Words: ");
	print_int_padded(words, 0);
	print("  Chars: ");
	print_int_padded(chars, 0);
	print("\n");
}
