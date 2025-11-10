#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>

#define MAX_ARGS 10

static int split_args(char *cmd, char *args[], int max_args) {
	int count = 0;
	char *p = cmd;

	while (*p && count < max_args - 1) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == '\0')
			break;

		args[count++] = p;

		while (*p && *p != ' ' && *p != '\t')
			p++;

		if (*p != '\0') {
			*p = '\0';
			p++;
		}
	}
	args[count] = NULL;
	return count;
}

void pipes_cmd(char *input) {
	char *pipe_pos = strchr(input, '|');
	if (!pipe_pos)
		return;

	*pipe_pos = '\0';
	char *cmd1 = trim(input);
	char *cmd2 = trim(pipe_pos + 1);

	int64_t pipe_id = my_pipe_open("terminal_pipe");
	if (pipe_id < 0) {
		print_format("[Pipe Error] Cannot create pipe\n");
		return;
	}

	char id_buffer[16];
	intToString((int) pipe_id, id_buffer);

	// ---- writer (cmd1)
	char cmd1_name[50];
	first_token(cmd1, cmd1_name, sizeof(cmd1_name));
	char *argv1[] = {cmd1_name, id_buffer, "write", NULL};

	// ---- reader (cmd2)
	char *tokens[MAX_ARGS];
	int argc2 = split_args(cmd2, tokens, MAX_ARGS);
	if (argc2 == 0) {
		print_format("[Pipe Error] Empty second command.\n");
		return;
	}
	char *cmd2_name = tokens[0];

	char *argv2[MAX_ARGS];
	int j;
	for (j = 0; j < argc2; j++)
		argv2[j] = tokens[j];
	argv2[j++] = id_buffer;
	argv2[j++] = (char *) "read";
	argv2[j] = NULL;

	void *func1 = find_function(cmd1_name);
	void *func2 = find_function(cmd2_name);

	if (func1 == NULL || func2 == NULL) {
		print_format("[Pipe Error] One of the commands is invalid.\n");
		my_pipe_close(pipe_id);
		return;
	}

	print_format("[DEBUG] Creating pipe %d between '%s' -> '%s'\n", (int) pipe_id, cmd1_name, cmd2_name);

	// ⚠️ crear primero el reader, luego el writer
	int pid_reader = create_process(cmd2_name, func2, j, argv2, 128);
	int pid_writer = create_process(cmd1_name, func1, 3, argv1, 128);

	if (pid_reader < 0 || pid_writer < 0) {
		print_format("[Pipe Error] Failed to create processes.\n");
		return;
	}

	waitpid(pid_writer);
	waitpid(pid_reader);

	print_format("[DEBUG] pipe %d finished.\n", (int) pipe_id);

	// IMPORTANTE: no cerrar desde el shell; lo cierran los procesos.
	// my_pipe_close(pipe_id);
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

	print_format("\n[Pipe Help] Command '%s' not found or not supported for a pipe.\n", cmd);
	print_format("Available pipe commands:\n");
	print_format("  - cat    : stdin -> stdout / pipe\n");
	print_format("  - wc     : counts lines/words/chars\n");
	print_format("  - filter : filters lines containing a word\n\n");
	return NULL;
}
