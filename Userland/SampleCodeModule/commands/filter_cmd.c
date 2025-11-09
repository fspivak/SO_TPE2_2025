#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

#define MAX_LINE 128

void filter_cmd(int argc, char **argv) {
	if (argc < 2) {
		print("Usage: filter <word>\n");
		return;
	}

	char *keyword = argv[1];
	int pipe_mode = 0;
	int pipe_id = -1;

	// Si viene desde un pipe: argv[2] = id, argv[3] = "read"
	if (argc >= 4 && strcmp(argv[3], "read") == 0) {
		pipe_mode = 1;
		pipe_id = satoi(argv[2]);
	}

	char buffer[MAX_LINE];
	int idx = 0;
	char c;

	if (pipe_mode) {
		while (my_pipe_read(pipe_id, &c, 1) > 0) {
			if (c == '\n' || idx >= MAX_LINE - 1) {
				buffer[idx] = '\0';
				if (strstr(buffer, keyword) != NULL) {
					print(buffer);
					print("\n");
				}
				idx = 0;
			}
			else {
				buffer[idx++] = c;
			}
		}
	}
	else {
		// desde teclado
		print("Enter text (Ctrl+D to finish):\n");
		while ((c = getchar()) != -1) {
			putchar(c);
			if (c == '\n' || idx >= MAX_LINE - 1) {
				buffer[idx] = '\0';
				if (strstr(buffer, keyword) != NULL) {
					print(buffer);
					print("\n");
				}
				idx = 0;
			}
			else {
				buffer[idx++] = c;
			}
		}
	}
}
