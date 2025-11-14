#include "../include/stinUser.h"
#include "include/syscall.h"
#include "include/test_util.h"
#include <stddef.h>

enum State { RUNNING, BLOCKED, KILLED };

// Wrapper para endless_loop con firma correcta
static void endless_loop_wrapper(int argc, char **argv) {
	(void) argc;
	(void) argv;
	endless_loop(); // Loop infinito sin prints
}

typedef struct P_rq {
	int32_t pid;
	enum State state;
} p_rq;

int64_t test_processes(uint64_t argc, char *argv[]) {
	uint8_t rq;
	uint8_t alive = 0;
	uint8_t action;
	uint64_t max_processes;
	char *argvAux[] = {"endless_loop", NULL};
	if (argc != 1)
		return -1;

	if ((max_processes = satoi(argv[0])) <= 0)
		return -1;

	p_rq p_rqs[max_processes];

	while (1) {
		// Create max_processes processes
		for (rq = 0; rq < max_processes; rq++) {
			print_format("\n entro");
			p_rqs[rq].pid = my_create_process("endless_loop", endless_loop_wrapper, 1, argvAux);
			if (p_rqs[rq].pid == -1) {
				print_format("test_process: ERROR creating process\n");
				return -1;
			}
			else {
				p_rqs[rq].state = RUNNING;
				alive++;
			}
		}

		// Randomly kills, blocks or unblocks processes until every one has been killed
		while (alive > 0) {
			for (rq = 0; rq < max_processes; rq++) {
				action = GetUniform(100) % 2;

				switch (action) {
					case 0:
						if (p_rqs[rq].state == RUNNING || p_rqs[rq].state == BLOCKED) {
							print_format("\n mato");
							if (my_kill(p_rqs[rq].pid) == -1) {
								print_format("test_process: ERROR killing process\n");
								return -1;
							}
							p_rqs[rq].state = KILLED;
							alive--;
						}
						break;

					case 1:
						if (p_rqs[rq].state == RUNNING) {
							print_format("\nbloqueo");
							if (my_block(p_rqs[rq].pid) == -1) {
								print_format("test_process: ERROR blocking process\n");
								return -1;
							}
							p_rqs[rq].state = BLOCKED;
						}
						break;
				}
			}

			// Randomly unblocks processes
			for (rq = 0; rq < max_processes; rq++)
				if (p_rqs[rq].state == BLOCKED && GetUniform(100) % 2) {
					// print_format("\ndesbloqueo");
					if (my_unblock(p_rqs[rq].pid) == -1) {
						print_format("test_process: ERROR unblocking process\n");
						return -1;
					}
					p_rqs[rq].state = RUNNING;
				}
		}
		// my_yield();
	}
	print_format("salgo");
	return 1;
}
