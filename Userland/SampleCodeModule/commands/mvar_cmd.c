#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/syscall.h"
#include "../tests/include/syscall.h"
#include "../tests/include/test_util.h"

#define MAX_WRITERS 10
#define MAX_READERS 10

// Estado del MVar
static char shared_var = 0; // Variable compartida
static int is_empty = 1;	// 1 = vacía, 0 = ocupada

// ============================================================
// Proceso escritor  (firma correcta: (int,char**))
// ============================================================
// void writer_proc(int argc, char **argv) {
//     int id = *(int *)argv[0];
//     char value = 'A' + id;

//     while (1) {
//         sleepUser(500 + (id * 200)); // espera "aleatoria"

//         sem_wait("empty");
//         sem_wait("mutex");
//         sleepUser(2000);

//         // ---- Sección crítica ----
//         shared_var = value;
//         is_empty = 0;
//         print_format("Escritor %c escribió valor: %c\n", 'A' + id, value);
//         // -------------------------

//         sem_post("mutex");
//         sem_post("full");
//         sleepUser(50);
//     }
// }

// // ============================================================
// // Proceso lector  (firma correcta: (int,char**))
// // ============================================================
// void reader_proc(int argc, char **argv) {
//     int id = *(int *)argv[0];

//     while (1) {
//         sleepUser(600 + (id * 150)); // espera "aleatoria"

//         sem_wait("full");
//         sem_wait("mutex");
//         sleepUser(2000);

//         // ---- Sección crítica ----
//         char value = shared_var;
//         shared_var = 0;
//         is_empty = 1;
//         print_format("Lector %d leyó valor: %c\n", id, value);
//         // -------------------------

//         sem_post("mutex");
//         sem_post("empty");
//         sleepUser(50);
//     }
// }

// ============================================================
// Función auxiliar para generar nombre tipo "writer_X"
// ============================================================
void build_name(char *dest, const char *prefix, int id) {
	int len = 0;
	while (prefix[len] != '\0') {
		dest[len] = prefix[len];
		len++;
	}
	dest[len++] = (char) ('0' + id);
	dest[len] = '\0';
}

////////////////////////////////////////////////////////

// ============================================================
// Proceso escritor
// ============================================================
void writer_proc(int argc, char **argv) {
	int id = *(int *) argv[0];
	char value = 'A' + id;

	for (int k = 0; k < 8; k++) {
		sleepUser(500 + (id * 200));

		sem_wait("empty");
		sem_wait("mutex");

		shared_var = value;
		is_empty = 0;
		print_format("Escritor %c escribió valor: %c\n", 'A' + id, value);

		sem_post("mutex");
		sem_post("full");
	}

	print_format("Escritor %c finalizó.\n", 'A' + id);
	exit_process();
}

// ============================================================
// Proceso lector
// ============================================================
void reader_proc(int argc, char **argv) {
	int id = *(int *) argv[0];

	for (int k = 0; k < 8; k++) {
		sleepUser(600 + (id * 150));

		sem_wait("full");
		sem_wait("mutex");

		char value = shared_var;
		shared_var = 0;
		is_empty = 1;
		print_format("Lector %d leyó valor: %c\n", id, value);

		sem_post("mutex");
		sem_post("empty");
	}

	print_format("Lector %d finalizó.\n", id);
	exit_process();
}

///////////////////////////////////////////////////////////////////////

// ============================================================
// Comando principal
// ============================================================
void mvar_cmd(int argc, char **argv) {
	if (argc < 3) {
		print_format("Uso: mvar <cant_escritores> <cant_lectores>\n");
		return;
	}

	int writers = satoi(argv[1]);
	int readers = satoi(argv[2]);

	if (writers <= 0 || readers <= 0 || writers > MAX_WRITERS || readers > MAX_READERS) {
		print_format("Error: cantidad inválida (máx 10 de cada tipo)\n");
		return;
	}

	// Crear semáforos si no existen (en kernel, compartidos entre procesos)
	sem_open("mutex", 1);
	sem_open("empty", 1);
	sem_open("full", 0);

	// ids persistentes en memoria válida
	static int writer_ids[MAX_WRITERS];
	static int reader_ids[MAX_READERS];

	// Crear escritores
	for (int i = 0; i < writers; i++) {
		char name[16];
		build_name(name, "writer_", i);

		writer_ids[i] = i;

		char *args[1];
		args[0] = (char *) &writer_ids[i];
		create_process(name, (void *) writer_proc, 1, args, 128);
	}

	// Crear lectores
	for (int i = 0; i < readers; i++) {
		char name[16];
		build_name(name, "reader_", i);

		reader_ids[i] = i;

		char *args[1];
		args[0] = (char *) &reader_ids[i];
		create_process(name, (void *) reader_proc, 1, args, 128);
	}

	print_format("MVar iniciado correctamente.\n");
	print_format("Lectores y escritores ejecutándose en paralelo.\n");
}
