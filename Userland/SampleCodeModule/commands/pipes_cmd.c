#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

#define MAX_PIPE_ARGS 8
#define MAX_PIPE_ARG_LEN 64

static int tokenize_command(const char *command, char storage[][MAX_PIPE_ARG_LEN], char *argv[], int max_tokens) {
	if (command == NULL || storage == NULL || argv == NULL || max_tokens <= 0) {
		return 0;
	}

	int count = 0;
	const char *ptr = command;

	while (*ptr != '\0' && count < max_tokens) {
		while (*ptr == ' ') {
			ptr++;
		}

		if (*ptr == '\0') {
			break;
		}

		int len = 0;
		while (ptr[len] != '\0' && ptr[len] != ' ' && len < MAX_PIPE_ARG_LEN - 1) {
			storage[count][len] = ptr[len];
			len++;
		}
		storage[count][len] = '\0';
		argv[count] = storage[count];
		count++;

		ptr += len;
		while (*ptr != '\0' && *ptr != ' ') {
			ptr++;
		}
	}

	return count;
}

void pipes_cmd(char *input) {
	if (command_is_background_mode()) {
		print_format("ERROR: pipes does not support background yet\n");
		return;
	}

	char *pipe_pos = strchr(input, '|');
	if (!pipe_pos)
		return;

	*pipe_pos = '\0';
	char *cmd1 = trim(input);
	char *cmd2 = trim(pipe_pos + 1);

	int64_t pipe_id = my_pipe_open("terminal_pipe");
	if (pipe_id < 0) {
		print_format("ERROR: Failed to create pipe\n");
		return;
	}

	char argv1_storage[MAX_PIPE_ARGS + 3][MAX_PIPE_ARG_LEN];
	char *argv1_tokens[MAX_PIPE_ARGS + 3];
	int argc1_base = tokenize_command(cmd1, argv1_storage, argv1_tokens, MAX_PIPE_ARGS);

	char argv2_storage[MAX_PIPE_ARGS + 3][MAX_PIPE_ARG_LEN];
	char *argv2_tokens[MAX_PIPE_ARGS + 3];
	int argc2_base = tokenize_command(cmd2, argv2_storage, argv2_tokens, MAX_PIPE_ARGS);

	if (argc1_base == 0 || argc2_base == 0) {
		print_format("ERROR: Invalid pipe commands\n");
		my_pipe_close(pipe_id);
		return;
	}

	void *func1 = find_function(argv1_tokens[0]);
	void *func2 = find_function(argv2_tokens[0]);

	if (func1 == NULL || func2 == NULL) {
		print_format("[Pipe Error] One of the commands is invalid.\n");
		my_pipe_close(pipe_id);
		return;
	}

	char id_buffer_writer[16];
	char id_buffer_reader[16];
	intToString((int) pipe_id, id_buffer_writer);
	intToString((int) pipe_id, id_buffer_reader);

	argv1_tokens[argc1_base] = id_buffer_writer;
	argv1_tokens[argc1_base + 1] = "write";
	argv1_tokens[argc1_base + 2] = NULL;

	argv2_tokens[argc2_base] = id_buffer_reader;
	argv2_tokens[argc2_base + 1] = "read";
	argv2_tokens[argc2_base + 2] = NULL;

	int pid_writer = create_process(argv1_tokens[0], func1, argc1_base + 2, argv1_tokens, 128);
	int pid_reader = create_process_foreground(argv2_tokens[0], func2, argc2_base + 2, argv2_tokens, 128);

	if (pid_writer < 0 || pid_reader < 0) {
		print_format("Error creating processes for pipe\n");
		my_pipe_close(pipe_id);
		return;
	}

	waitpid(pid_writer);
	waitpid(pid_reader);

	my_pipe_close(pipe_id);
}

void *find_function(char *cmd) {
	if (!strcmp(cmd, "cat")) {
		return cat_cmd;
	}
	if (!strcmp(cmd, "wc")) {
		return wc_cmd;
	}
	if (!strcmp(cmd, "filter")) {
		return filter_cmd;
	}
	if (!strcmp(cmd, "ps")) {
		return ps_cmd;
	}

	// Si el comando no se reconoce o no es apto para pipe
	print_format("\n[Pipe Help] Command '%s' not available for pipe.\n", cmd);
	print_format("Supported pipe commands:\n");
	print_format("  - cat : reads from stdin and writes to stdout\n");
	print_format("  - wc  : counts lines and words from stdin\n");
	print_format("  - ps  : prints process information\n\n");
	print_format("  - filter : filters lines containing a word\n");

	return NULL;
}
