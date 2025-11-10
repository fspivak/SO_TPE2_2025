#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
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

static void (*find_pipe_function(const char *cmd, int is_writer))(int, char **);

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

	void (*writer_func)(int, char **) = find_pipe_function(argv1_tokens[0], 1);
	void (*reader_func)(int, char **) = find_pipe_function(argv2_tokens[0], 0);

	if (writer_func == NULL || reader_func == NULL) {
		print_format("[Pipe Help] One of the pipe commands is not supported in this position.\n");
		print_format("Supported pipe combinations:\n");
		print_format("  - cat | wc\n");
		print_format("  - cat | filter\n");
		print_format("  - filter | wc\n");
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

	int pid_reader = create_process_with_io(argv2_tokens[0], reader_func, argc2_base + 2, argv2_tokens, 128, NULL);
	if (pid_reader < 0) {
		print_format("Error creating reader process for pipe\n");
		my_pipe_close(pipe_id);
		return;
	}

	int pid_writer =
		create_process_foreground_with_io(argv1_tokens[0], writer_func, argc1_base + 2, argv1_tokens, 128, NULL);
	if (pid_writer < 0) {
		print_format("Error creating writer process for pipe\n");
		waitpid(pid_reader);
		my_pipe_close(pipe_id);
		return;
	}

	waitpid(pid_writer);
	waitpid(pid_reader);
	clear_foreground(pid_writer);
	set_foreground(getpid());

	my_pipe_close(pipe_id);
}

static void (*find_pipe_function(const char *cmd, int is_writer))(int, char **) {
	if (!strcmp(cmd, "cat")) {
		return is_writer ? cat_pipe_main : NULL;
	}
	if (!strcmp(cmd, "wc")) {
		return (!is_writer) ? wc_pipe_main : NULL;
	}
	if (!strcmp(cmd, "filter")) {
		return is_writer ? filter_pipe_writer_main : filter_pipe_main;
	}
	return NULL;
}
