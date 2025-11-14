// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

void help_main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	print_format("=== Available Commands ===\n");
	print_format("  help              - Show this help message\n");
	print_format("  man               - Show manual for specific command\n");
	print_format("  clear             - Clear screen\n");
	print_format("  clock             - Show current time\n");
	print_format("  exit              - Exit terminal\n");
	print_format("  ps                - List all processes\n");
	print_format("  mem               - Show memory status\n");
	print_format("  sh                - Create a new shell\n");
	print_format("  getpid            - Get process ID\n");
	print_format("  loop <seconds>    - Loop command with a given interval\n");
	print_format("  cat               - Display input (stdin) to output or write to a pipe\n");
	print_format("  wc                - Count lines, words and characters from input or pipe\n");
	print_format("  filter            - It filters out the vowels from input or pipe\n");
	print_format("  <cmd1> | <cmd2>   - Pipe commands (see 'man pipe' for available commands)\n");
	print_format("  mvar              - See 'man mvar' for details\n");
	print_format("  test_mm           - See 'man test_mm' for details\n");
	print_format("  test_process      - See 'man test_process' for details\n");
	print_format("  test_sync         - See 'man test_sync' for details\n");
	print_format("  test_prio         - See 'man test_prio' for details\n");
}

void help_cmd(int argc, char **argv) {
	int pid_help = command_spawn_process("help", help_main, 0, NULL, 1, NULL);
	if (pid_help < 0) {
		print_format("ERROR: Failed to create process help\n");
		return;
	}

	command_handle_child_process(pid_help, "help");
}