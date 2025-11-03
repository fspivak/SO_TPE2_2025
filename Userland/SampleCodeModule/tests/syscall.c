#include "syscall.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stdint.h>

// Declaración forward para waitpid
int waitpid(int pid);

int64_t my_create_process(char *name, void *function, uint64_t argc, char *argv[]) {
	return (int64_t) create_process(name, function, (int) argc, argv, 128);
}

int64_t my_getpid() {
	return (int64_t) getpid();
}

int64_t my_kill(uint64_t pid) {
	return (int64_t) kill((int) pid);
}

int64_t my_block(uint64_t pid) {
	return (int64_t) block((int) pid);
}

int64_t my_unblock(uint64_t pid) {
	return (int64_t) unblock((int) pid);
}

int64_t my_nice(uint64_t pid, uint64_t newPrio) {
	return (int64_t) nice((int) pid, (int) newPrio);
}

int64_t my_yield() {
	yield();
	return 0;
}

// Stubs para semaforos (Fase 4 - no implementada)
int64_t my_sem_open(char *sem_id, uint64_t initialValue) {
	(void) sem_id;
	(void) initialValue;
	return -1;
}

int64_t my_sem_wait(char *sem_id) {
	(void) sem_id;
	return -1;
}

int64_t my_sem_post(char *sem_id) {
	(void) sem_id;
	return -1;
}

int64_t my_sem_close(char *sem_id) {
	(void) sem_id;
	return -1;
}

int64_t my_wait(int64_t pid) {
	return (int64_t) waitpid((int) pid);
}
