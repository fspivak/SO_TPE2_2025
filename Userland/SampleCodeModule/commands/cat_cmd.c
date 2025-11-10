// // #include <stdint.h>
// // #include <stdio.h>
// // #include "../include/commands.h"
// // #include "../include/libasmUser.h"
// // #include "../include/stringUser.h"

// // typedef enum { STDIN = 0, STDOUT = 1, STDERR = 2 } FDS;

// // void cat_main(int argc, char *argv[]) {
// //     char buffer[256];
// //     int bytesRead;

// //     // Si no hay argumentos, leer stdin
// //     if (argc == 1) {
// //         while (1) {
// //             read(STDIN, buffer, sizeof(buffer));
// //             print(buffer);
// //         }
// //     } else {
// //         // Si hay argumentos, aún no hay FS, mostramos error o simulamos
// //         for (int i = 1; i < argc; i++) {
// //             print("cat: no file system implemented\n");
// //         }
// //     }

// //     return;
// // }

// // void cat_cmd(int argc, char **argv) {
// // 	int pid_cat = create_process("cat", cat_main, 1, argv, 1);
// // 	if (!validate_create_process_error("cat", pid_cat)) {
// // 		return;
// // 	}

// // 	waitpid(pid_cat);
// // }

// #include <stdint.h>
// #include "../include/commands.h"
// #include "../include/libasmUser.h"
// #include "../include/stinUser.h"
// #include "../include/stringUser.h"

// // file descriptors estándar (igual que en tu kernel)
// typedef enum { STDIN = 0, STDOUT = 1, STDERR = 2 } FDS;

// void cat_main(int argc, char *argv[]) {
//     char buffer[256];
//     uint64_t bytesRead;

//     // Sin argumentos: leer desde stdin (pipe o teclado)
//     if (argc == 1) {
//         while (1) {
//             print("entro");
//             read(STDIN, buffer, sizeof(buffer));  // bloqueante
//             // if (bytesRead == 0) {
//             //     // EOF (pipe cerrado)
//             //     break;
//             // }
//             print(buffer);
//         }
//     } else {
//         // Todavía no hay FS, mostramos error
//         for (int i = 1; i < argc; i++) {
//             print("cat: no file system implemented\n");
//         }
//     }
// }

// void cat_cmd(int argc, char **argv) {
//     int pid_cat = create_process("cat", cat_main, 1, argv, 1);
//     if (!validate_create_process_error("cat", pid_cat))
//         return;

//     waitpid(pid_cat);
// }

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h" // por getchar()
#include "../include/stringUser.h"
#include <stdint.h>

void cat_main(int argc, char *argv[]) {
	char c;

	print_format("Entrando a cat (Ctrl+D para EOF)\n");

	while (1) {
		c = getchar(); // 🔹 esto ya lee del STDIN del proceso

		if (c == 0)
			continue; // sin input aún

		if (c == -1) { // Ctrl+D
			print_format("\n[EOF]\n");
			return;
		}

		// simplemente reenviamos el carácter a STDOUT
		putchar(c);
	}
}

void cat_cmd(int argc, char **argv) {
	int pid_cat = create_process("cat", cat_main, argc, argv, 1);
	if (!validate_create_process_error("cat", pid_cat))
		return;

	waitpid(pid_cat);
}
