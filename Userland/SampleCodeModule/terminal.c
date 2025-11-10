#include "include/terminal.h"
#include "include/commands.h"
#include "include/format_utils.h"
#include "include/libasmUser.h"
#include "include/process_entries.h"
#include "include/screen.h"
#include "include/snake.h"
#include "include/stinUser.h"
#include "include/stringUser.h"
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
	print_format("\nWelcome to x64 BareBones OS\n");
	print_format("Type 'help' for available commands\n\n");
	print_format(">  "); /* Prompt inicial */

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
			print_format("\n"); /* Nueva linea despues del comando */

			if (!strcmp(buffer, "help")) {
				help_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "zoom in")) {
				print_format("Zoom not available in VGA text mode\n");
			}
			else if (!strcmp(buffer, "zoom out")) {
				print_format("Zoom not available in VGA text mode\n");
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
				print_format("Snake not available in VGA text mode\n");
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
			else if (!strcmp(buffer, "test_prio")) {
				test_prio_cmd(0, NULL);
			}
			else if (startsWith(buffer, "test_prio ")) {
				execute_command_with_args(buffer, "test_prio ", 10, test_prio_cmd);
			}
			else if (!strcmp(buffer, "sh")) {
				create_new_shell();
			}
			else if (!strcmp(buffer, "kill")) {
				kill_cmd(0, NULL);
			}
			else if (startsWith(buffer, "kill ")) {
				execute_command_with_args(buffer, "kill ", 5, kill_cmd);
			}
			else if (!strcmp(buffer, "loop")) {
				loop_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "nice")) {
				nice_cmd(0, NULL);
			}
			else if (startsWith(buffer, "nice ")) {
				execute_command_with_args(buffer, "nice ", 5, nice_cmd);
			}
			else if (!strcmp(buffer, "block")) {
				block_cmd(0, NULL);
			}
			else if (startsWith(buffer, "block ")) {
				execute_command_with_args(buffer, "block ", 6, block_cmd);
			}
			else if (!strcmp(buffer, "cat")) {
				cat_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "wc")) {
				wc_cmd(0, NULL);
			}
			else if (c == 4) { // Ctrl+D
				print_format("\n[EOF]\n");
				// close_fd(0); // 🔹 (tu syscall que cierre el descriptor 0, STDIN)
				break; // termina la lectura actual
			}
			// Detectar comando con pipe
			else if (strchr(buffer, '|') != NULL) {
				pipes_cmd(buffer);
			}
			else if (startsWith(buffer, "filter ")) {
				execute_command_with_args(buffer, "filter ", 7, filter_cmd);
			}
			else if (!strcmp(buffer, "filter")) {
				filter_cmd(0, NULL);
			}
			else if (!strcmp(buffer, "mvar")) {
				print_format("Uso: mvar <writers> <readers>\n");
			}
			else if (startsWith(buffer, "mvar ")) {
				execute_command_with_args(buffer, "mvar ", 5, mvar_cmd);
			}

			//////////////////TODO: borrar estos tests/////////////////////////////

			else if (!strcmp(buffer, "test_pipe")) {
				test_pipe_cmd(0, NULL);
			}
			///////////////////////////////////////////////////////////////////////////////
			else if (i > 0) { /* Solo mostrar error si se escribio algo */
				print_format("Command '%s' not found\n", buffer);
			}

			print_format(">  "); /* Mostrar prompt para siguiente comando */
			// previousLength=i;
			tabs = 0;
			i = 0;
		}
	}
}

void create_new_shell() {
	char *argv[] = {NULL};

	print_format("Creating new shell: \n");
	int pid = create_process("shellCreated", (void *) terminal, 0, argv, 128);
	if (pid < 0) {
		print_format("Error: could not create clock process\n");
		return;
	}
	waitpid(pid);
	return;
}

void exit_shell() {
	exit_process();
}

void callClock() {
	char *argv[] = {NULL};
	int pid = create_process("clock", (void *) clock_entry, 0, argv, 1);
	if (pid < 0) {
		print_format("Error: could not create clock process\n");
		return;
	}

	waitpid(pid);
	return;
}
