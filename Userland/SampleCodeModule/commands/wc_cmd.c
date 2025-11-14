// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>
#include <stdint.h>

static void wc_print_result(int lines, int words, int chars) {
	print_format("\nLines: %d Words: %d Chars: %d\n", lines, words, chars);
}

// Funcion principal de wc que funciona tanto en modo directo como en pipes
// Usa read_input() para transparencia de I/O - no necesita conocer file descriptors
// Procesa mientras lee (como cat) en lugar de almacenar en buffer (como filter)
// Esto es mas eficiente en memoria y funciona mejor con pipes
void wc_main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	int lines = 0;
	int words = 0;
	int chars = 0;
	int in_word = 0;
	char c;
	int bytes_read;

	// Leer caracter por caracter y procesar inmediatamente (como cat)
	// Esto es mas eficiente que almacenar en buffer y procesar al final
	// Funciona bien tanto en modo directo como en pipes
	// Los comandos no necesitan conocer los file descriptors - read_input los oculta
	while ((bytes_read = read_input(&c, 1)) > 0) {
		if (c == '\0') {
			continue;
		}

		// Ignorar backspace - no cuenta como caracter
		if (c == '\b' || c == 8) {
			continue;
		}

		// Contar caracteres
		chars++;

		// Contar lineas
		if (c == '\n') {
			lines++;
			in_word = 0;
		}
		// Contar palabras - detectar espacios y tabs
		else if (c == ' ' || c == '\t') {
			in_word = 0;
		}
		// Caracter valido - puede ser inicio de palabra
		else {
			if (!in_word) {
				in_word = 1;
				words++;
			}
		}
	}

	// Cuando termina la lectura (Ctrl+D o EOF), mostrar resultados
	// Esto funciona tanto en modo directo (muestra resultados al final) como en pipes
	wc_print_result(lines, words, chars);

	// Dar tiempo a que la escritura al pipe se complete antes de terminar. Esto asegura que filter reciba los datos
	// antes de que wc se desregistre del pipe
	yield();
}

void wc_cmd(int argc, char **argv) {
	int pid_wc = command_spawn_process("wc", wc_main, argc, argv, 128, NULL);
	if (pid_wc < 0) {
		print_format("ERROR: Failed to create process wc\n");
		return;
	}

	command_handle_child_process(pid_wc, "wc");
}
