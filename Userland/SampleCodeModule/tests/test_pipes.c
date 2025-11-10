// #include "../include/libasmUser.h"
// #include "../include/stinUser.h"
// #include "include/syscall.h" // donde declaraste my_pipe_open etc.
// #include <stddef.h>

// //////////////// TODO: Borrar este test entero (todo el archivo)//////////////////
// void writer_process(int argc, char **argv) {
// 	int id = my_pipe_open("demo");
// 	char *msg = "Hola desde el escritor!\n";

// 	for (int i = 0; i < 5; i++) {
// 		my_pipe_write(id, msg, 24);
// 		print("[Writer] Mensaje enviado\n");
// 		my_yield(); // alternar con lector
// 	}

// 	my_pipe_close(id);
// }

// // Proceso lector
// void reader_process(int argc, char **argv) {
// 	int id = my_pipe_open("demo");
// 	char buf[64] = {0};

// 	for (int i = 0; i < 5; i++) {
// 		my_pipe_read(id, buf, 24);
// 		print("[Reader] Recibido: ");
// 		print(buf);
// 		my_yield();
// 	}

// 	my_pipe_close(id);
// }

// // Comando principal de prueba
// void test_pipe_cmd(int argc, char **argv) {
// 	print("\n=== INICIO TEST PIPE ===\n");

// 	char *args[] = {NULL};
// 	int writer_pid = my_create_process("writer", writer_process, 0, args);
// 	int reader_pid = my_create_process("reader", reader_process, 0, args);

// 	waitpid(writer_pid);
// 	waitpid(reader_pid);

// 	print("Se lanzaron procesos reader y writer.\n");
// 	print("Cada uno alterna escritura y lectura en el pipe 'demo'.\n");
// 	return;
// }

////////////////////////////////////SOLO ESCRIBE EL READER, PARA VER QEU LE LLEGUEN BIEN LAS COSAS/////////////////

// #include "../include/libasmUser.h"
// #include "../include/stinUser.h"
// #include "include/syscall.h"
// #include <stddef.h>
// #include <stdint.h>

// #define MSG           "Hola desde el escritor!\n"
// #define MSG_LEN       24
// #define LINES_TO_READ 5

// // ========== Writer: solo escribe, no imprime ==========
// void writer_process(int argc, char **argv) {
//     int id = my_pipe_open("demo");

//     // Enviar LINES_TO_READ líneas terminadas en '\n'
//     for (int i = 0; i < LINES_TO_READ; i++) {
//         // Escribimos exactamente MSG_LEN bytes (incluye '\n')
//         my_pipe_write(id, MSG, MSG_LEN);
//         my_yield();
//     }

//     my_pipe_close(id);
// }

// // ========== Reader: lee lineas completas (hasta '\n') y imprime ==========
// static int read_line_from_pipe(int pipe_id, char *out, int maxlen) {
//     // Lee de a 1 byte hasta '\n' o hasta llenar el buffer (maxlen-1 para '\0')
//     int i = 0;
//     for (; i < maxlen - 1; i++) {
//         char c = 0;
//         // leer 1 byte del pipe (bloqueante con semáforo)
//         my_pipe_read(pipe_id, &c, 1);
//         if (c == '\n') {
//             out[i] = '\0';
//             return i;           // largo de la linea sin '\n'
//         }
//         out[i] = c;
//     }
//     out[maxlen - 1] = '\0';
//     return i; // cortado por longitud
// }

// void reader_process(int argc, char **argv) {
//     int id = my_pipe_open("demo");

//     // Mostrar el id del pipe que abrió el reader (debug útil)
//     print("[Reader] Abrió pipe id = ");
//     printBase((uint64_t)id, 10);
//     print("\n");

//     char buf[64];

//     for (int i = 0; i < LINES_TO_READ; i++) {
//         int n = read_line_from_pipe(id, buf, sizeof(buf));
//         print("[Reader] Recibido: ");
//         print(buf);
//         print("\n");
//         my_yield();
//     }

//     my_pipe_close(id);
// }

// // ========== Comando principal ==========
// void test_pipe_cmd(int argc, char **argv) {
//     print("\n=== INICIO TEST PIPE (LIMPIO) ===\n");

//     char *args[] = { NULL };
//     int writer_pid = my_create_process("writer", writer_process, 0, args);
//     int reader_pid = my_create_process("reader", reader_process, 0, args);

//     // Esperar a que terminen (si tu waitpid devuelve 0/1, usalo como lo venías usando)
//     waitpid(writer_pid);
//     waitpid(reader_pid);

//     print("Se lanzaron procesos reader y writer.\n");
//     print("Solo el reader imprime los mensajes recibidos desde el pipe 'demo'.\n");
// }

/////////////////////////////////Test con multiples escritores///////////////////
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "include/syscall.h"
#include <stddef.h>
#include <stdint.h>

#define LINES_PER_WRITER 3
#define MSG_LEN 64

// 🔧 Prototipo de stpcpy (para evitar error de tipo)
char *stpcpy(char *dest, const char *src);

// ======================== HELPERS ========================
static int intToStr(int num, char *str) {
	int i = 0, j;
	if (num == 0) {
		str[i++] = '0';
		str[i] = '\0';
		return 1;
	}
	char tmp[10];
	while (num > 0) {
		tmp[i++] = (num % 10) + '0';
		num /= 10;
	}
	for (j = 0; j < i; j++)
		str[j] = tmp[i - j - 1];
	str[i] = '\0';
	return i;
}

// Construye un mensaje tipo “[Writer X] Mensaje Y desde el escritor!\n”
static int build_message(char *buf, int writer_num, int msg_num) {
	char numbuf1[10], numbuf2[10];
	intToStr(writer_num, numbuf1);
	intToStr(msg_num, numbuf2);

	char *ptr = buf;

	ptr = stpcpy(ptr, "[Writer ");
	ptr = stpcpy(ptr, numbuf1);
	ptr = stpcpy(ptr, "] Mensaje ");
	ptr = stpcpy(ptr, numbuf2);
	ptr = stpcpy(ptr, " desde el escritor!\n");

	return (int) (ptr - buf);
}

// Implementación local de stpcpy
char *stpcpy(char *dest, const char *src) {
	while (*src)
		*dest++ = *src++;
	*dest = '\0';
	return dest;
}

// ======================== WRITERS ========================
void writer_process(int argc, char **argv) {
	int id = my_pipe_open("demo");
	int num = (argc > 0 && argv[0] != NULL) ? argv[0][0] - '0' : 0;

	char msg[MSG_LEN];
	for (int i = 0; i < LINES_PER_WRITER; i++) {
		int len = build_message(msg, num, i + 1);
		my_pipe_write(id, msg, len);
		my_yield();
	}

	my_pipe_close(id);
}

// ======================== READER ========================
static int read_line_from_pipe(int pipe_id, char *out, int maxlen) {
	int i = 0;
	for (; i < maxlen - 1; i++) {
		char c = 0;
		my_pipe_read(pipe_id, &c, 1);
		if (c == '\n') {
			out[i] = '\0';
			return i;
		}
		out[i] = c;
	}
	out[maxlen - 1] = '\0';
	return i;
}

void reader_process(int argc, char **argv) {
	int id = my_pipe_open("demo");
	print_format("[Reader] Abrio pipe id = %d\n", id);

	char buf[MSG_LEN];
	// Espera todos los mensajes (2 writers × 3 líneas = 6)
	for (int i = 0; i < 2 * LINES_PER_WRITER; i++) {
		read_line_from_pipe(id, buf, sizeof(buf));
		print_format("[Reader] Recibido: %s\n", buf);
		my_yield();
	}

	my_pipe_close(id);
}

// ======================== COMANDO PRINCIPAL ========================
void test_pipe_cmd(int argc, char **argv) {
	print_format("\n=== INICIO TEST PIPE MULTI (2 WRITERS, 1 READER) ===\n");

	char *args1[] = {"1", NULL};
	char *args2[] = {"2", NULL};
	char *argsR[] = {NULL};

	int writer1_pid = my_create_process("writer1", writer_process, 1, args1);
	int writer2_pid = my_create_process("writer2", writer_process, 1, args2);
	int reader_pid = my_create_process("reader", reader_process, 0, argsR);

	waitpid(writer1_pid);
	waitpid(writer2_pid);
	waitpid(reader_pid);

	print_format("Finalizo test de 2 writers escribiendo concurrentemente en el pipe 'demo'.\n");
}

/////////////////////////////////TEST printeando los dos//////////

// #include "../include/libasmUser.h"
// #include "../include/stinUser.h"
// #include "include/syscall.h"
// #include <stddef.h>
// #include <stdint.h>

// #define MSGS 5
// #define MSG_LEN 64

// // Semáforo global para proteger la terminal
// #define PRINT_SEM "print_lock"

// // ======================== HELPERS ========================

// // Convierte un número entero a string (sin signo)
// static int intToStr(int num, char *str) {
//     int i = 0, j;
//     if (num == 0) {
//         str[i++] = '0';
//         str[i] = '\0';
//         return 1;
//     }
//     char tmp[10];
//     while (num > 0) {
//         tmp[i++] = (num % 10) + '0';
//         num /= 10;
//     }
//     for (j = 0; j < i; j++)
//         str[j] = tmp[i - j - 1];
//     str[i] = '\0';
//     return i;
// }

// // Implementación local de stpcpy
// char *stpcpy(char *dest, const char *src) {
//     while (*src)
//         *dest++ = *src++;
//     *dest = '\0';
//     return dest;
// }

// // Construye un mensaje del writer
// static int build_message(char *buf, int idx) {
//     char num[10];
//     intToStr(idx, num);
//     char *ptr = buf;
//     ptr = stpcpy(ptr, "Hola desde writer! Mensaje ");
//     ptr = stpcpy(ptr, num);
//     ptr = stpcpy(ptr, "\n");
//     return (int)(ptr - buf);
// }

// // ======================== PROCESOS ========================

// void writer_process(int argc, char **argv) {
//     int id = my_pipe_open("ttytest");
//     char msg[MSG_LEN];

//     for (int i = 0; i < MSGS; i++) {
//         int len = build_message(msg, i + 1);
//         my_pipe_write(id, msg, len);

//         my_sem_wait(PRINT_SEM);
//         print("[Writer] Envió mensaje: ");
//         print(msg);
//         my_sem_post(PRINT_SEM);

//         my_yield();
//     }

//     my_pipe_close(id);
// }

// void reader_process(int argc, char **argv) {
//     int id = my_pipe_open("ttytest");
//     char buf[MSG_LEN] = {0};

//     for (int i = 0; i < MSGS; i++) {
//         my_pipe_read(id, buf, 40); // lee un mensaje completo aprox

//         my_sem_wait(PRINT_SEM);
//         print("[Reader] Recibió: ");
//         print(buf);
//         my_sem_post(PRINT_SEM);

//         my_yield();
//     }

//     my_pipe_close(id);
// }

// // ======================== COMANDO PRINCIPAL ========================

// void test_pipe_cmd(int argc, char **argv) {
//     print("\n=== INICIO TEST PIPE + TERMINAL (SIN PISARSE) ===\n");

//     // Crear semáforo global para proteger prints
//     my_sem_open(PRINT_SEM, 1);

//     char *args[] = { NULL };
//     int writer_pid = my_create_process("writer_print", writer_process, 0, args);
//     int reader_pid = my_create_process("reader_print", reader_process, 0, args);

//     waitpid(writer_pid);
//     waitpid(reader_pid);

//     my_sem_close(PRINT_SEM);
//     print("\nFinalizó test pipe + terminal.\n");
// }
