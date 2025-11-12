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
	{NULL, NULL}};

void man_cmd(int argc, char **argv) {
	if (argc < 2) {
		print_format("Uso: man <comando>\nComandos disponibles: test_mm, test_process, test_sync, test_prio\n");
		return;
	}

	const char *cmd = argv[1];
	for (int i = 0; manual_entries[i].name != NULL; i++) {
		if (strcmp(manual_entries[i].name, cmd) == 0) {
			print_format("%s\n", manual_entries[i].description);
			return;
		}
	}

	print_format("No hay manual disponible para '%s'\n", cmd);
}
