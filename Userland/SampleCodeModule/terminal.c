#include "include/terminal.h"
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
				help();
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
			else if (!strcmp(buffer, "exit")) {
				print("Goodbye!\n");
				sound(2);
				sleepUser(20);
				exit(); /* Halt del sistema */
			}
			else if (!strcmp(buffer, "snake")) {
				print("Snake not available in VGA text mode\n");
			}
			else if (!strcmp(buffer, "clock")) {
				clock();
			}
			else if (!strcmp(buffer, "clear")) {
				/* Llamar a syscall para limpiar pantalla */
				clearScreen();
			}
			else if (buffer[0] == 't' && buffer[1] == 'e' && buffer[2] == 's' && buffer[3] == 't' && buffer[4] == '_' &&
					 buffer[5] == 'm' && buffer[6] == 'm') {
				/* Parsear parametro opcional */
				char *size_param = NULL;
				if (buffer[7] == ' ') {
					size_param = &buffer[8];
				}

				if (size_param && size_param[0] != '\0') {
					print("Running test_mm with custom size: ");
					print(size_param);
					print(" bytes\n");
					char *argv[] = {size_param, NULL};
					test_mm(1, argv);
				}
				else {
					print("Running test_mm with default size: 1MB\n");
					char *argv[] = {"1048576", NULL}; /* 1MB default */
					test_mm(1, argv);
				}
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

void help() {
	print("\n=== Available Commands ===\n\n");
	print("General:\n");
	print("  help              - Show this help message\n");
	print("  clear             - Clear screen\n");
	print("  clock             - Show current time\n");
	print("  showRegisters     - Display CPU registers\n");
	print("  exit              - Exit terminal\n");
	print("\nMemory Manager Tests:\n");
	print("  test_mm           - Run memory manager test (default: 1MB)\n");
	print("  test_mm <size>    - Run memory manager test with custom size\n");
	print("                      Example: test_mm 2097152 (2MB)\n");
	print("\n");
}

void refreshScreen() {
	for (int i = 0; i <= screenHeight; i++) {
		for (int j = 0; j <= screenWidth; j++) {
			putPixel(0, j, i);
		}
	}
}
