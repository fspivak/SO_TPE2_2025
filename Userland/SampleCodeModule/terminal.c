#include "include/terminal.h"
#include "include/commands.h"
#include "include/format_utils.h"
#include "include/libasmUser.h"
#include "include/process_entries.h"
#include "include/screen.h"
#include "include/snake.h"
#include "include/stinUser.h"
#include "include/stringUser.h"
#include "tests/include/test_util.h"
#include <stddef.h>
#include <stdint.h>

#define STARTING_POSITION_X 0
#define STARTING_POSITION_Y 0

#define MAX_ZOOM 3
#define MIN_ZOOM 1

/* Configuracion de pantalla por zoom level */
int charsPerLine[] = {128, 64, 42};
int charSize = 1;
int screenWidth;
int screenHeight;
int lastRunHeight = 0;

void terminal() {
	char buffer[1000];
	char c;
	int i = 0;
	int tabs = 0;
	sound(1);
	getScreenSize(&screenWidth, &screenHeight);

	/* Mensaje de bienvenida */
	print("\nWelcome to x64 BareBones OS\n");
	print("Type 'help' for available commands\n\n");
	print(">  "); /* Prompt inicial */

	while (1) {
		if ((c = getchar()) != '\n') {
			if (c == 8) {
				if (i > 0) {
					if (buffer[i - 1] == '\t') {
						tabs--;
						putchar(c);
						putchar(c);
						putchar(c);
					}
					putchar(c);
					i--;
				}
				// moveCursor();
				// actualizarPantalla();
			}
			else if (c == '\t') {
				if (i + tabs * 3 + 4 < charsPerLine[charSize - 1] * 2) {
					tabs++;
					buffer[i++] = c;
					putchar(c);
				}
			}
			else {
				if ((i + tabs * 3 + 1) < charsPerLine[charSize - 1] * 2 && c != 0) {
					buffer[i++] = c;
					putchar(c);
				}
				// actualizarPantalla();
			}
		}
		else {
			buffer[i] = 0;
			print("\n"); /* Nueva linea despues del comando */

			if (!strcmp(buffer, "help")) {
				help_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "zoom in")) {
				print("Zoom not available in VGA text mode\n");
			}
			else if (!strcmp(buffer, "zoom out")) {
				print("Zoom not available in VGA text mode\n");
			}
			else if (!strcmp(buffer, "showRegisters")) {
				imprimirRegistros();
			}
			else if (!strcmp(buffer, "ps")) {
				ps_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "getpid")) {
				getpid_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "exit")) {
				exit_shell();
			}
			else if (!strcmp(buffer, "snake")) {
				print("Snake not available in VGA text mode\n");
			}
			else if (!strcmp(buffer, "clock")) {
				callClock();
			}
			else if (!strcmp(buffer, "clear")) {
				clear_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "mem")) {
				mem_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "test_ab")) {
				test_ab_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "test_mm")) {
				test_mm_cmd(0, NULL);
			}
			else if (startsWith(buffer, "test_mm ")) {
				execute_command_with_args(buffer, "test_mm ", 8, test_mm_cmd);
			}
			else if (!strcmp(buffer, "test_process")) {
				test_process_cmd(0, NULL);
			}
			else if (startsWith(buffer, "test_process ")) {
				execute_command_with_args(buffer, "test_process ", 13, test_process_cmd);
			}
			else if (startsWith(buffer, "test_sync ")) {
				execute_command_with_args(buffer, "test_sync ", 10, test_sync_cmd);
			}
			else if (!strcmp(buffer, "sh")) {
				create_new_shell();
			}
			else if (startsWith(buffer, "kill ")) {
				/// TODO: ver si esto está bien
				execute_command_with_args(buffer, "kill", 5, kill_process);
			}
			//////////////////TODO: borrar estos tests/////////////////////////////
			else if (startsWith(buffer, "test_jero")) {
				// Extraer argumentos si los hay
				char *args = NULL;
				if (buffer[9] == ' ') {
					args = &buffer[10]; // Saltar el espacio
				}
				run_test_jero(args);
			}
			else if (!strcmp(buffer, "test_pipe")) {
				test_pipe_cmd(0, NULL);
			}
			///////////////////////////////////////////////////////////////////////////////
			else if (i > 0) { /* Solo mostrar error si se escribio algo */
				print("Command '");
				print(buffer);
				print("' not found\n");
			}

			print(">  "); /* Mostrar prompt para siguiente comando */
			// previousLength=i;
			tabs = 0;
			i = 0;
		}
	}
}

void clean(int ammount) {
	for (int j = 0; j < ammount; j++) {
		putchar(' ');
	}
}

void refreshScreen() {
	for (int i = 0; i <= screenHeight; i++) {
		for (int j = 0; j <= screenWidth; j++) {
			putPixel(0, j, i);
		}
	}
}

void show_current_pid() {
	int pid = getpid();
	print("Current PID: ");
	printBase(pid, 10);
	print("\n");
}

void run_test_process(char *args) {
	char *argv[] = {args};
	uint64_t argc = (args != NULL && args[0] != '\0') ? 1 : 0;

	void test_process_entry(uint64_t argc, char *argv[]);

	int pid = create_process("test_process", (void *) test_process_entry, argc, argv, 1);
	if (pid < 0) {
		print("Error: could not create test process\n");
		return;
	}

	// TODO: BORRAR ESTE PRINT///
	print("Created process 'test_process' with PID ");
	printBase(pid, 10);
	print("\n");

	// // TODO: Ver si queremos que la terminal espere a que termine el test:
	waitpid(pid);
	return;
}

void run_test_ab() {
	char *argv[] = {NULL};

	int pid = create_process("test_ab", (void *) test_ab_entry, 0, argv, 1);
	if (pid < 0) {
		print("Error: could not create AB test process\n");
		return;
	}

	print("AB test process created with PID ");
	printBase(pid, 10);
	print("\n");

	waitpid(pid);
	return;
}

void create_new_shell() {
	char *argv[] = {NULL};

	print("Creating new shell: \n");
	int pid = create_process("shellCreated", (void *) terminal, 0, argv, 1);
	if (pid < 0) {
		print("Error: could not create clock process\n");
		return;
	}
	waitpid(pid);
	return;
}

void exit_shell() {
	int pid = getpid();
	kill(pid);
}

void callClock() {
	char *argv[] = {NULL};
	int pid = create_process("clock", (void *) clock_entry, 0, argv, 1);
	if (pid < 0) {
		print("Error: could not create clock process\n");
		return;
	}

	// // TODO: Ver si queremos que la terminal espere a que termine el test:
	waitpid(pid);
	return;
}

void run_test_mm(char *args) {
	char *argv[] = {args};
	uint64_t argc = (args != NULL && args[0] != '\0') ? 1 : 0;

	int pid = create_process("test_mm", (void *) test_mm_entry, argc, argv, 1);
	if (pid < 0) {
		print("Error: could not create Memory Manager test process\n");
		return;
	}

	print("Memory Manager test process created with PID ");
	printBase(pid, 10);
	print("\n");

	// // TODO: Ver si queremos que la terminal espere a que termine el test:
	waitpid(pid);
	return;
}

void kill_process(int argc, char *argv[]) {
	if (argc < 2 || argv[1] == NULL || argv[1][0] == '\0' || satoi(argv[1]) <= 0) {
		print("Error: missing PID argument\n");
		return;
	}
	int pid = 0;
	pid = satoi(argv[1]);
	if (kill(pid) <= 0) {
		print("Error: invalid PID: ");
		printBase(pid, 10);
		print("\n");
		return;
	}
}

//////////////////////TODO: BORRAR ESTE TEST////////////////////////
void testJERO(uint64_t argc, char *argv[]) {
	int num = 0;
	while (1) {
		printBase(num, 10);
		print(" ");
		num = (num + 1) % 10;
		sleepUser(10);
	}
}

void run_test_jero(char *args) {
	print("\n=== Running JERO Test ===\n");
	char *argv[] = {NULL};
	int pid = create_process("testJERO", (void *) testJERO, 1, argv, 1);
	waitpid(pid);
	print("\n=== JERO Test Completed ===\n\n");
}
