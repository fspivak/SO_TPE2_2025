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

static int is_space_char(char value) {
	return value == ' ' || value == '\t' || value == '\r';
}

static int sanitize_background_flag(char *buffer) {
	if (buffer == NULL) {
		return 0;
	}

	int end = 0;
	while (buffer[end] != '\0') {
		end++;
	}

	end--;

	while (end >= 0 && is_space_char(buffer[end])) {
		buffer[end] = '\0';
		end--;
	}

	if (end >= 0 && buffer[end] == '&') {
		buffer[end] = '\0';
		end--;
		while (end >= 0 && is_space_char(buffer[end])) {
			buffer[end] = '\0';
			end--;
		}
		return 1;
	}

	return 0;
}

void terminal() {
	char buffer[1000];
	char c;
	int i = 0;
	int tabs = 0;
	sound(1);
	getScreenSize(&screenWidth, &screenHeight);

	/* Mensaje de bienvenida */
	print_format("\nWelcome to x64 BareBones OS\n");
	print_format("Type 'help' for available commands\n");
	print_format("Pipes: use 'cmd1 | cmd2' with cat, wc, filter, ps\n");
	print_format("Background: append '&' to run commands without blocking the terminal\n\n");
	set_foreground(getpid());
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

			int run_in_background = sanitize_background_flag(buffer);
			command_set_background_mode(run_in_background);

			char *command = buffer;
			while (command != NULL && is_space_char(*command)) {
				command++;
			}

			if (!strcmp(command, "help")) {
				help_cmd(0, NULL);
			}
			else if (!strcmp(command, "zoom in")) {
				print_format("Zoom not available in VGA text mode\n");
			}
			else if (!strcmp(command, "zoom out")) {
				print_format("Zoom not available in VGA text mode\n");
			}
			else if (!strcmp(command, "showRegisters")) {
				imprimirRegistros();
			}
			else if (!strcmp(command, "ps")) {
				ps_cmd(0, NULL);
			}
			else if (!strcmp(command, "getpid")) {
				getpid_cmd(0, NULL);
			}
			else if (!strcmp(command, "exit")) {
				exit_shell();
			}
			else if (!strcmp(command, "snake")) {
				print_format("Snake not available in VGA text mode\n");
			}
			else if (!strcmp(command, "clock")) {
				callClock();
			}
			else if (!strcmp(command, "clear")) {
				clear_cmd(0, NULL);
			}
			else if (!strcmp(command, "mem")) {
				mem_cmd(0, NULL);
			}
			else if (!strcmp(command, "test_ab")) {
				test_ab_cmd(0, NULL);
			}
			else if (!strcmp(command, "test_mm")) {
				test_mm_cmd(0, NULL);
			}
			else if (startsWith(command, "test_mm ")) {
				execute_command_with_args(command, "test_mm ", 8, test_mm_cmd);
			}
			else if (!strcmp(command, "test_process")) {
				test_process_cmd(0, NULL);
			}
			else if (startsWith(command, "test_process ")) {
				execute_command_with_args(command, "test_process ", 13, test_process_cmd);
			}
			else if (startsWith(command, "test_sync ")) {
				execute_command_with_args(command, "test_sync ", 10, test_sync_cmd);
			}
			else if (!strcmp(command, "test_prio")) {
				test_prio_cmd(0, NULL);
			}
			else if (startsWith(command, "test_prio ")) {
				execute_command_with_args(command, "test_prio ", 10, test_prio_cmd);
			}
			else if (!strcmp(command, "sh")) {
				if (run_in_background) {
					print_format("ERROR: sh cannot be run in background\n");
				}
				else {
					create_new_shell();
				}
			}
			else if (!strcmp(command, "kill")) {
				kill_cmd(0, NULL);
			}
			else if (startsWith(command, "kill ")) {
				execute_command_with_args(command, "kill ", 5, kill_cmd);
			}
			else if (!strcmp(command, "loop")) {
				loop_cmd(0, NULL);
			}
			else if (startsWith(command, "loop ")) {
				execute_command_with_args(command, "loop ", 5, loop_cmd);
			}
			else if (!strcmp(command, "nice")) {
				nice_cmd(0, NULL);
			}
			else if (startsWith(command, "nice ")) {
				execute_command_with_args(command, "nice ", 5, nice_cmd);
			}
			else if (!strcmp(command, "block")) {
				block_cmd(0, NULL);
			}
			else if (startsWith(command, "block ")) {
				execute_command_with_args(command, "block ", 6, block_cmd);
			}
			else if (!strcmp(command, "cat")) {
				cat_cmd(0, NULL);
			}
			else if (!strcmp(command, "wc")) {
				wc_cmd(0, NULL);
			}
			else if (c == 4) { // Ctrl+D
				print_format("\n[EOF]\n");
				// close_fd(0); // 🔹 (tu syscall que cierre el descriptor 0, STDIN)
				break; // termina la lectura actual
			}
			// Detectar comando con pipe
			else if (strchr(command, '|') != NULL) {
				pipes_cmd(command);
			}
			else if (startsWith(command, "filter ")) {
				execute_command_with_args(command, "filter ", 7, filter_cmd);
			}
			else if (!strcmp(command, "filter")) {
				filter_cmd(0, NULL);
			}

			//////////////////TODO: borrar estos tests/////////////////////////////

			else if (!strcmp(command, "test_pipe")) {
				test_pipe_cmd(0, NULL);
			}
			///////////////////////////////////////////////////////////////////////////////
			else if (command[0] != '\0') {
				print_format("Command '%s' not found\n", command);
			}

			command_reset_background_mode();

			int bg_pid = -1;
			const char *bg_name = NULL;
			if (command_pop_background_notification(&bg_pid, &bg_name)) {
				yield();
				print_format("Process %s running in background (PID: %d)\n", bg_name, bg_pid);
			}

			yield();
			print_format(">  "); /* Mostrar prompt para siguiente comando */
			// previousLength=i;
			tabs = 0;
			i = 0;
		}
	}
}

void create_new_shell() {
	char *argv[] = {NULL};

	print_format("Creating new shell...\n");
	int pid = command_spawn_process("shellCreated", (void *) terminal, 0, argv, 128, NULL);
	if (pid < 0) {
		print_format("ERROR: Failed to create shell\n");
		return;
	}
	command_handle_child_process(pid, "shell");
}

void exit_shell() {
	exit_process();
}

void callClock() {
	char *argv[] = {NULL};
	int pid = command_spawn_process("clock", (void *) clock_entry, 0, argv, 1, NULL);
	if (pid < 0) {
		print_format("ERROR: Failed to create clock process\n");
		return;
	}

	command_handle_child_process(pid, "clock");
}
