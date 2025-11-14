#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>
#include <stdint.h>

#define CAT_BUFFER_SIZE 1024

// Funcion unificada que funciona tanto en modo directo como en pipes
// Segun el enunciado: "Todo proceso debera ser capaz de escribir/leer tanto de un pipe
// como de la pantalla sin necesidad de que su codigo sea modificado"
// La transparencia se logra usando read_input() y write_output() que ocultan los file descriptors
// y automaticamente redirigen a pipe o pantalla segun la configuracion del proceso
// Los comandos NO deben verificar el tipo de IO ni conocer los file descriptors, solo leer y escribir
// Leer caracter por caracter y escribir inmediatamente para echo en tiempo real
void cat_main(int argc, char **argv) {
	(void) argc;
	(void) argv;
	char c;
	int bytes_read;

	// Leer caracter por caracter y escribir inmediatamente
	// Esto da echo en tiempo real cuando lee del teclado
	// y funciona correctamente con pipes (aunque menos eficiente)
	// Los comandos no necesitan conocer los file descriptors - las funciones
	// read_input y write_output los ocultan y las syscalls determinan automaticamente
	// de donde leer y a donde escribir segun la configuracion del proceso
	while ((bytes_read = read_input(&c, 1)) > 0) {
		if (c == '\0') {
			continue;
		}
		// Escribir inmediatamente sin conocer el destino
		// Las syscalls write se encargan de la redireccion automatica
		write_output(&c, 1, 0x00ffffff, 0);
	}
}

// cat_cmd crea un proceso separado para permitir transparencia de I/O
// Esto permite que cat funcione tanto en modo directo (heredando FDs del terminal)
// como en pipes (con configuracion de I/O personalizada)
// El mismo codigo cat_main funciona sin modificacion en ambos casos
void cat_cmd(int argc, char **argv) {
	int pid_cat = command_spawn_process("cat", cat_main, argc, argv, 100, NULL);
	if (pid_cat < 0) {
		print_format("ERROR: Failed to create process cat\n");
		return;
	}

	command_handle_child_process(pid_cat, "cat");
}
