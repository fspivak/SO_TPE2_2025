#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>

void pipes_cmd(char *input) {
	char *pipe_pos = strchr(input, '|');
	if (!pipe_pos)
		return;

	*pipe_pos = '\0';
	char *cmd1 = trim(input);
	char *cmd2 = trim(pipe_pos + 1);

	int64_t pipe_id = my_pipe_open("terminal_pipe");
	if (pipe_id < 0) {
		print("Error creating pipe\n");
		return;
	}

	// argv para ambos procesos: les pasamos el id del pipe
	char id_buffer[10];
	intToString((int) pipe_id, id_buffer);
	char *argv1[] = {cmd1, id_buffer, "write", NULL};
	char *argv2[] = {cmd2, id_buffer, "read", NULL};

	void *func1 = find_function(cmd1);
	void *func2 = find_function(cmd2);

	if (func1 == NULL || func2 == NULL) {
		print("[Pipe Error] One of the commands is invalid.\n");
		my_pipe_close(pipe_id);
		return;
	}

	int pid_writer = create_process(cmd1, func1, 3, argv1, 8);
	int pid_reader = create_process(cmd2, func2, 3, argv2, 8);

	if (pid_writer < 0 || pid_reader < 0) {
		print("Error creating processes for pipe\n");
		my_pipe_close(pipe_id);
		return;
	}

	waitpid(pid_writer);
	waitpid(pid_reader);

	my_pipe_close(pipe_id);
}

void *find_function(char *cmd) {
	if (!strcmp(cmd, "cat"))
		return cat_cmd;
	// if (!strcmp(cmd, "wc")) return wc_cmd;
	if (!strcmp(cmd, "ps"))
		return ps_cmd;

	// Si el comando no se reconoce o no es apto para pipe
	print("\n[Pipe Help] Command '");
	print(cmd);
	print("' not found or not supported for a pipe.\n");

	print("Available pipe commands:\n");
	print("  - cat : reads from stdin and writes to stdout\n");
	// print("  - wc  : counts lines/words from stdin\n"); // si lo agregás después
	print("  - ps  : prints process information (stdout only)\n\n");
	return NULL;
}
