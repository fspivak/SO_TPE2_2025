#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "include/syscall.h" // donde declaraste my_pipe_open etc.
#include <stddef.h>

//////////////// TODO: Borrar este test entero (todo el archivo)//////////////////
void writer_process(int argc, char **argv) {
	int id = my_pipe_open("demo");
	char *msg = "Hola desde el escritor!\n";

	for (int i = 0; i < 5; i++) {
		my_pipe_write(id, msg, 24);
		print("[Writer] Mensaje enviado\n");
		my_yield(); // alternar con lector
	}

	my_pipe_close(id);
}

// Proceso lector
void reader_process(int argc, char **argv) {
	int id = my_pipe_open("demo");
	char buf[64] = {0};

	for (int i = 0; i < 5; i++) {
		my_pipe_read(id, buf, 24);
		print("[Reader] Recibido: ");
		print(buf);
		my_yield();
	}

	my_pipe_close(id);
}

// Comando principal de prueba
void test_pipe_cmd(int argc, char **argv) {
	print("\n=== INICIO TEST PIPE ===\n");

	char *args[] = {NULL};
	my_create_process("writer", writer_process, 0, args);
	my_create_process("reader", reader_process, 0, args);

	print("Se lanzaron procesos reader y writer.\n");
	print("Cada uno alterna escritura y lectura en el pipe 'demo'.\n");
}
