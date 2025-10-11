#include "syscall.h"
#include "../include/stinUser.h"
#include "include/test_util.h"
#include <stdint.h>

// Declaraciones de syscalls (en syscalls.c)
int create_process(const char *name, void (*entry)(int, char **), int argc, char **argv, int priority);
int getpid(void);
int kill(int pid);
int block(int pid);
int unblock(int pid);
int nice(int pid, int priority);
void yield(void);

static void endless_loop_wrapper(int argc, char **argv) {
	(void) argc;
	(void) argv;
	endless_loop();
}

int64_t my_create_process(char *name, uint64_t argc, char *argv[]) {
	return (int64_t) create_process(name, endless_loop_wrapper, (int) argc, argv, 128);
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
	(void) pid;
	return -1; // waitpid not implemented yet
}
