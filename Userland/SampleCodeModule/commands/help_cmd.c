#include "../include/libasmUser.h"
#include "../include/screen.h"
#include "../include/stinUser.h"
#include <stdint.h>

void help_cmd(int argc, char **argv) {
	print("\n=== Available Commands ===\n\n");
	print("General:\n");
	print("  help              - Show this help message\n");
	print("  clear             - Clear screen\n");
	print("  clock             - Show current time\n");
	print("  showRegisters     - Display CPU registers\n");
	print("  exit              - Exit terminal\n");
	print("\nProcess Management:\n");
	print("  ps                - List all processes\n");
	print("  getpid            - Show current process ID\n");
	print("\nMemory:\n");
	print("  mem               - Show memory status\n");
	print("\nMemory Manager Tests:\n");
	print("  test_mm           - Run memory manager test (default: 1MB)\n");
	print("  test_mm <size>    - Run memory manager test with custom size\n");
	print("                          Example: test_mm 524288 (512KB)\n");
	print("  test_process      - Run process management test (default: 3 processes)\n");
	print("  test_process <n>  - Run process management test with n processes (1-64)\n");
	print("                          Example: test_process 5\n");
	print("  test_ab           - Run simple AB test (two processes printing A and B)\n");
	print("\n");
}
