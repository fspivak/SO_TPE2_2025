#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>
#include <stdint.h>

extern uint64_t test_sync(uint64_t argc, char *argv[]);

void test_sync_main(int argc, char **argv) {
	// test_sync requiere 2 argumentos: n (iteraciones) y use_sem (0 o 1)
	// argv[0] es "test_sync", argv[1] es n, argv[2] es use_sem
	if (argc < 3 || argv == NULL || argv[1] == NULL || argv[2] == NULL) {
		print("test_sync: Usage: test_sync <n> <use_sem>\n");
		print("  n: Number of iterations\n");
		print("  use_sem: 1 to use semaphores, 0 to test without semaphores\n");
		print("  Example: test_sync 1000 1\n");
		return;
	}

	// test_sync espera recibir los argumentos directamente (n y use_sem)
	char *args[] = {argv[1], argv[2], NULL};

	int64_t result = test_sync(2, args);

	if (result != 0) {
		print("test_sync: ERROR occurred during test\n");
	}
}

void test_sync_cmd(int argc, char **argv) {
	int pid_test = my_create_process("test_sync", test_sync_main, argc, argv);

	if (pid_test < 0) {
		print("ERROR: Failed to create process test_sync\n");
		return;
	}

	waitpid(pid_test);
}
