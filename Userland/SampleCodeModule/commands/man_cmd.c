// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include <stddef.h>

typedef struct {
	const char *name;
	const char *description;
} ManualEntry;

static ManualEntry manual_entries[] = {
	{"test_mm", "\ntest_mm - Tests the Memory Manager.\n"
				"Usage:\n"
				"  test_mm             - Runs the test with default size (1MB)\n"
				"  test_mm <size>      - Runs the test with a custom size (in bytes)\n"
				"Example:\n"
				"  test_mm 524288      - Allocates and frees 512KB\n"},
	{"test_process", "\ntest_process - Tests process creation and scheduling.\n"
					 "Usage:\n"
					 "  test_process        - Runs the test with 3 processes\n"
					 "  test_process <n>    - Runs the test with n processes (1–64)\n"
					 "Example:\n"
					 "  test_process 5\n"},
	{"test_sync", "\ntest_sync - Tests synchronization using semaphores.\n"
				  "Usage:\n"
				  "  test_sync <n> <use_sem>\n"
				  "    n: number of increments\n"
				  "    use_sem: 1 to use semaphores, 0 to run without them\n"
				  "Example:\n"
				  "  test_sync 1000 1\n"},
	{"test_prio", "\ntest_prio - Tests priority-based scheduling.\n"
				  "Usage:\n"
				  "  test_prio <max_value>\n"
				  "Example:\n"
				  "  test_prio 1000000\n"
				  "Or:\n"
				  "  test_prio           for use default value (100000)\n"},
	{"mvar", "\nmvar - MVar Writers and Readers synchronization.\n"
			 "Usage:\n"
			 "  mvar <writers> <readers>\n"
			 "    writers: number of writers to spawn\n"
			 "    readers: number of readers to spawn\n"
			 "Example:\n"
			 "  mvar 2 2\n"},
	{"pipe", "\npipe - Pipes allow connecting two commands.\n"
			 "Usage:\n"
			 "  <command1> | <command2>\n"
			 "    command1: writer command (outputs to pipe)\n"
			 "    command2: reader command (reads from pipe)\n"
			 "\n"
			 "Available commands for pipes:\n"
			 "  cat, wc, filter, ps, help, man, mem\n"
			 "\n"
			 "Examples:\n"
			 "  ps | cat        - List processes and display them\n"
			 "  cat | wc        - Count lines, words and characters from input\n"
			 "  ps | filter     - List processes and filter vowels\n"
			 "  help | wc       - Count help message lines\n"},
	{NULL, NULL}};

// Funcion principal de man que funciona tanto en modo directo como en pipes
void man_main(int argc, char **argv) {
	if (argc < 2) {
		print_format(
			"Uso: man <comando>\nComandos disponibles: test_mm, test_process, test_sync, test_prio, mvar, pipe\n");
		return;
	}

	const char *cmd = argv[1];

	// Caso especial para pipe: imprimir en partes para evitar cortes
	if (strcmp(cmd, "pipe") == 0) {
		print_format("\npipe - Pipes allow connecting two commands.\n");
		print_format("Usage:\n");
		print_format("  <command1> | <command2>\n");
		print_format("    command1: writer command (outputs to pipe)\n");
		print_format("    command2: reader command (reads from pipe)\n");
		print_format("\n");
		print_format("Available commands for pipes:\n");
		print_format("  cat, wc, filter, ps, help, man, mem\n");
		print_format("\n");
		print_format("Examples:\n");
		print_format("  ps | cat        - List processes and display them\n");
		print_format("  cat | wc        - Count lines, words and characters from input\n");
		print_format("  ps | filter     - List processes and filter vowels\n");
		print_format("  help | wc       - Count help message lines\n");
		print_format("\n");
		print_format("Note: Test commands (test_mm, test_process, test_sync, test_prio)\n");
		print_format("      cannot be used in pipes.\n");
		return;
	}

	for (int i = 0; manual_entries[i].name != NULL; i++) {
		if (strcmp(manual_entries[i].name, cmd) == 0) {
			print_format("%s\n", manual_entries[i].description);
			return;
		}
	}

	print_format("No hay manual disponible para '%s'\n", cmd);
}

void man_cmd(int argc, char **argv) {
	if (argc < 2) {
		print_format(
			"Uso: man <comando>\nComandos disponibles: test_mm, test_process, test_sync, test_prio, mvar, pipe\n");
		return;
	}

	const char *cmd = argv[1];

	// Caso especial para pipe: imprimir en partes para evitar cortes
	if (strcmp(cmd, "pipe") == 0) {
		print_format("\npipe - Pipes allow connecting two commands.\n");
		print_format("Usage:\n");
		print_format("  <command1> | <command2>\n");
		print_format("    command1: writer command (outputs to pipe)\n");
		print_format("    command2: reader command (reads from pipe)\n");
		print_format("\n");
		print_format("Available commands for pipes:\n");
		print_format("  cat, wc, filter, ps, help, man, mem\n");
		print_format("\n");
		print_format("Examples:\n");
		print_format("  ps | cat        - List processes and display them\n");
		print_format("  cat | wc        - Count lines, words and characters from input\n");
		print_format("  ps | filter     - List processes and filter vowels\n");
		print_format("  help | wc       - Count help message lines\n");
		print_format("\n");
		print_format("Note: Test commands (test_mm, test_process, test_sync, test_prio)\n");
		print_format("      cannot be used in pipes.\n");
		return;
	}

	for (int i = 0; manual_entries[i].name != NULL; i++) {
		if (strcmp(manual_entries[i].name, cmd) == 0) {
			print_format("%s\n", manual_entries[i].description);
			return;
		}
	}

	print_format("No hay manual disponible para '%s'\n", cmd);
}
