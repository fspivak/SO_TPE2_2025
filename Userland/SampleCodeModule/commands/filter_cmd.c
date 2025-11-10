#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>
#include <stdint.h>

#define FILTER_BUFFER_SIZE 1024

static int is_vowel(char c) {
	if (c >= 'A' && c <= 'Z') {
		c = (char) (c + ('a' - 'A'));
	}

	return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

static void filter_stream_from_pipe(uint64_t pipe_id) {
	char filtered[FILTER_BUFFER_SIZE];
	int length = 0;
	char c;

	while (my_pipe_read(pipe_id, &c, 1) > 0) {
		if (!is_vowel(c)) {
			if (length < FILTER_BUFFER_SIZE - 1) {
				filtered[length++] = c;
			}
		}
	}

	if (length > 0) {
		filtered[length] = '\0';
		putchar('\n');
		for (int i = 0; i < length; i++) {
			putchar(filtered[i]);
		}
		putchar('\n');
	}

	if (length == FILTER_BUFFER_SIZE - 1) {
		print_format("\n[filter] pipe output truncated to %d characters\n", FILTER_BUFFER_SIZE - 1);
	}
}

static void filter_stream_from_stdin(void) {
	char start_msg[] = "Filtering input (Ctrl+D for EOF)\n";
	int start_len = (int) strlen(start_msg);
	write(2, start_msg, start_len, 0x00ffffff, 0);

	char buffer[FILTER_BUFFER_SIZE];
	int length = 0;
	char c;

	while (read_bytes(0, &c, 1) > 0) {
		putchar(c);
		if (length < FILTER_BUFFER_SIZE - 1) {
			buffer[length++] = c;
		}
	}

	buffer[length] = '\0';

	char eof_msg[] = "\n[EOF]\n";
	int eof_len = (int) strlen(eof_msg);
	write(2, eof_msg, eof_len, 0x00ffffff, 0);

	for (int i = 0; i < length; i++) {
		if (!is_vowel(buffer[i])) {
			putchar(buffer[i]);
		}
	}

	if (length > 0) {
		putchar('\n');
	}

	if (length == FILTER_BUFFER_SIZE - 1) {
		print_format("\n[filter] input truncated to %d characters\n", FILTER_BUFFER_SIZE - 1);
	}
}

static void filter_stream_from_stdin_to_pipe(uint64_t pipe_id) {
	char start_msg[] = "Filtering input (Ctrl+D for EOF)\n";
	int start_len = (int) strlen(start_msg);
	write(2, start_msg, start_len, 0x00ffffff, 0);

	char buffer[FILTER_BUFFER_SIZE];
	int length = 0;
	char c;

	while (read_bytes(0, &c, 1) > 0) {
		putchar(c);
		if (length < FILTER_BUFFER_SIZE - 1) {
			buffer[length++] = c;
		}
	}

	char eof_msg[] = "\n[EOF]\n";
	int eof_len = (int) strlen(eof_msg);
	write(2, eof_msg, eof_len, 0x00ffffff, 0);

	buffer[length] = '\0';

	for (int i = 0; i < length; i++) {
		if (!is_vowel(buffer[i])) {
			if (my_pipe_write(pipe_id, &buffer[i], 1) < 0) {
				print_format("filter pipe: write error\n");
				break;
			}
		}
	}

	if (length == FILTER_BUFFER_SIZE - 1) {
		print_format("\n[filter] input truncated to %d characters\n", FILTER_BUFFER_SIZE - 1);
	}
}

void filter_main(int argc, char **argv) {
	(void) argc;
	(void) argv;
	filter_stream_from_stdin();
}

void filter_pipe_main(int argc, char **argv) {
	if (argc < 3 || argv == NULL) {
		print_format("filter pipe: invalid arguments\n");
		return;
	}

	int pipe_id = satoi(argv[argc - 2]);
	const char *mode = argv[argc - 1];

	if (pipe_id < 0 || mode == NULL || strcmp(mode, "read") != 0) {
		print_format("filter pipe: invalid pipe mode\n");
		return;
	}

	filter_stream_from_pipe((uint64_t) pipe_id);
	my_pipe_close((uint64_t) pipe_id);
}

void filter_pipe_writer_main(int argc, char **argv) {
	if (argc < 3 || argv == NULL) {
		print_format("filter pipe: invalid arguments\n");
		return;
	}

	int pipe_id = satoi(argv[argc - 2]);
	const char *mode = argv[argc - 1];

	if (pipe_id < 0 || mode == NULL || strcmp(mode, "write") != 0) {
		print_format("filter pipe: invalid pipe mode\n");
		return;
	}

	filter_stream_from_stdin_to_pipe((uint64_t) pipe_id);
	my_pipe_close((uint64_t) pipe_id);
}

void filter_cmd(int argc, char **argv) {
	if (argc > 1 && argv != NULL) {
		print_format("Usage: filter\n");
		return;
	}

	int pid_filter = command_spawn_process("filter", filter_main, argc, argv, 1, NULL);
	if (!validate_create_process_error("filter", pid_filter)) {
		return;
	}

	command_handle_child_process(pid_filter, "filter");
}