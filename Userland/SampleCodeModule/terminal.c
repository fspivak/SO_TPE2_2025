#include "include/terminal.h"
#include "../commands/process_entries.h"
#include "include/commands.h"
#include "include/libasmUser.h"
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
				show_current_pid();
			}
			else if (startsWith(buffer, "test_process")) {
				// Extraer argumentos si los hay
				char *args = NULL;
				if (buffer[12] == ' ') {
					args = &buffer[13]; // Saltar el espacio
				}
				run_test_process(args);
			}
			else if (!strcmp(buffer, "exit")) {
				/// TODO; el dia de mañana debería matar a la terminal///
				print("Goodbye!\n");
				sound(2);
				sleepUser(20);
				exit(); /* Halt del sistema */
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
				run_test_ab();
			}
			else if (startsWith(buffer, "test_mm")) {
				// Extraer argumentos si los hay
				char *args = NULL;
				if (buffer[7] == ' ') {
					args = &buffer[8]; // Saltar el espacio
				}
				run_test_mm(args);
			}

			// TODO: borrar este test
			else if (startsWith(buffer, "test_jero")) {
				// Extraer argumentos si los hay
				char *args = NULL;
				if (buffer[9] == ' ') {
					args = &buffer[10]; // Saltar el espacio
				}
				run_test_jero(args);
			}
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
	// waitpid(pid);
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

	// // TDOD: Ver si queremos que la terminal espere a que termine:
	//  waitpid(pid);
	return;
}

void callClock() {
	char *argv[] = {NULL};
	int pid = create_process("clock", (void *) clock_entry, 0, argv, 1);
	if (pid < 0) {
		print("Error: could not create clock process\n");
		return;
	}

	// TODO: BORRAR ESTE PRINT///
	print("Clock process created with PID ");
	printBase(pid, 10);
	print("\n");

	// // TODO: Ver si queremos que la terminal espere a que termine el test:
	// waitpid(pid);
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
	// waitpid(pid);
	return;
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
////////////////////////////////////////////////////////////////

void intToString(int value, char *buffer) {
	if (value == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	char temp[10];
	int pos = 0;
	int temp_value = value;

	if (temp_value < 0) {
		temp_value = -temp_value;
	}

	while (temp_value > 0) {
		temp[pos++] = '0' + (temp_value % 10);
		temp_value /= 10;
	}

	int buffer_pos = 0;
	if (value < 0) {
		buffer[buffer_pos++] = '-';
	}

	for (int i = pos - 1; i >= 0; i--) {
		buffer[buffer_pos++] = temp[i];
	}
	buffer[buffer_pos] = '\0';
}

int startsWith(const char *str, const char *prefix) {
	while (*prefix) {
		if (*str != *prefix) {
			return 0;
		}
		str++;
		prefix++;
	}
	return 1;
}
