// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>
#include <stdint.h>

#define FILTER_BUFFER_SIZE 1024

static int is_vowel(char c) {
	if (c >= 'A' && c <= 'Z') {
		c = (char) (c + ('a' - 'A'));
	}

	return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

void filter_main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	char buffer[FILTER_BUFFER_SIZE];
	int buffer_pos = 0;
	char read_buffer[256];
	int bytes_read;

	// Leer sin conocer el origen (teclado o pipe)
	// Las syscalls read se encargan de la redireccion automatica
	// Los comandos no necesitan conocer los file descriptors - read_input los oculta
	// Almacenar todo lo leido para filtrar al final (cuando se ejecuta individualmente)
	// o procesar inmediatamente (cuando esta en un pipe)
	while ((bytes_read = read_input(read_buffer, sizeof(read_buffer))) > 0) {
		for (int i = 0; i < bytes_read; i++) {
			char c = read_buffer[i];
			if (c == '\0') {
				continue;
			}

			// Ignorar backspace en el buffer
			if (c == '\b' || c == 8) {
				if (buffer_pos > 0) {
					buffer_pos--;
					buffer[buffer_pos] = '\0';
				}
				continue;
			}

			if (buffer_pos < FILTER_BUFFER_SIZE - 1) {
				buffer[buffer_pos++] = c;
				buffer[buffer_pos] = '\0';
			}
		}
	}

	// Cuando termina la lectura (Ctrl+D o EOF), filtrar y mostrar solo las vocales
	print_format("\n");
	for (int i = 0; i < buffer_pos; i++) {
		char c = buffer[i];
		if (is_vowel(c)) {
			write_output(&c, 1, 0x00ffffff, 0);
		}
	}
}

void filter_cmd(int argc, char **argv) {
	int pid_filter = command_spawn_process("filter", filter_main, argc, argv, 128, NULL);
	if (pid_filter < 0) {
		print_format("ERROR: Failed to create process filter\n");
		return;
	}

	command_handle_child_process(pid_filter, "filter");
}