#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

static void help_main(int argc, char **argv) {
	(void) argc; // No usamos (convencion)
	(void) argv; // No usamos (convencion)

	print_format("=== Available Commands ===\n");
	print_format("  help              - Show this help message\n");
	print_format("  clear             - Clear screen\n");
	print_format("  clock             - Show current time\n");
	print_format("  showRegisters     - Display CPU registers\n");
	print_format("  exit              - Exit terminal\n");
	print_format("  ps                - List all processes\n");
	print_format("  getpid            - Show current process ID\n");
	print_format("  mem               - Show memory status\n");
	print_format("  test_mm           - Run memory manager test (default: 1MB)\n");
	print_format("  test_mm <size>    - Run memory manager test with custom size\n");
	print_format("                          Example: test_mm 524288 (512KB)\n");
	print_format("  test_process      - Run process management test (default: 3 processes)\n");
	print_format("  test_process <n>  - Run process management test with n processes (1-64)\n");
	print_format("                          Example: test_process 5\n");
	print_format("  test_ab           - Run simple AB test (two processes printing A and B)\n");
	print_format("  test_sync <n> <use_sem> - Run synchronization test (semaphores)\n");
	print_format("                          Example: test_sync 1000 1 (with semaphores)\n");
	print_format("                          Example: test_sync 1000 0 (without semaphores)\n");
	print_format("  test_prio <max_value>   - Run priority test (verifies priority scheduling)\n");
	print_format("                          Example: test_prio 1000000\n");
	print_format("  sh                - Create a new shell\n");
	print_format("  loop              - Create a process that runs in infinite loop\n");
	print_format("  kill <pid>        - Kill the process with pid <pid>\n");
	print_format("  nice <pid> <prio> - Change priority of process <pid> to <prio> (0-255)\n");
	print_format("  block <pid>       - Block the process with pid <pid>\n");
	print_format("  cat               - Display input (stdin) to output or write to a pipe\n");
	print_format("  wc                - Count lines, words and characters from input or pipe\n");
	print_format("  filter <word>     - Show only lines containing <word> from input or pipe\n");
}

void help_cmd(int argc, char **argv) {
	int pid_help = command_spawn_process("help", help_main, 0, NULL, 1);
	if (pid_help < 0) {
		print_format("ERROR: Failed to create process help\n");
		return;
	}

	command_handle_child_process(pid_help, "help");
}