#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

static void help_main(int argc, char **argv) {
	(void) argc; // No usamos (convencion)
	(void) argv; // No usamos (convencion)

	print("=== Available Commands ===\n");
	print("  help              - Show this help message\n");
	print("  clear             - Clear screen\n");
	print("  clock             - Show current time\n");
	print("  showRegisters     - Display CPU registers\n");
	print("  exit              - Exit terminal\n");
	print("  ps                - List all processes\n");
	print("  getpid            - Show current process ID\n");
	print("  mem               - Show memory status\n");
	print("  test_mm           - Run memory manager test (default: 1MB)\n");
	print("  test_mm <size>    - Run memory manager test with custom size\n");
	print("                          Example: test_mm 524288 (512KB)\n");
	print("  test_process      - Run process management test (default: 3 processes)\n");
	print("  test_process <n>  - Run process management test with n processes (1-64)\n");
	print("                          Example: test_process 5\n");
	print("  test_ab           - Run simple AB test (two processes printing A and B)\n");
	print("  sh                - Create a new shell\n");
	print("  kill <pid>        - Kill the process with pid <pid>\n");
}

void help_cmd(int argc, char **argv) {
	int pid_help = create_process("help", help_main, 0, NULL, 4);
	if (pid_help < 0) {
		print("ERROR: Failed to create process help\n");
		return;
	}

	waitpid(pid_help);
}