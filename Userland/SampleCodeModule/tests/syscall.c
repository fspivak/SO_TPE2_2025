#include "include/syscall.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stdint.h>

// Declaraciones
int waitpid(int pid);
int sem_open(const char *name, uint32_t initial_value);
int sem_wait(const char *name);
int sem_post(const char *name);
int sem_close(const char *name);

int64_t my_create_process(char *name, void *function, uint64_t argc, char *argv[]) {
	return (int64_t) create_process(name, function, (int) argc, argv, 1);
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

int64_t my_sem_open(char *sem_id, uint64_t initialValue) {
	int result = sem_open(sem_id, (uint32_t) initialValue);
	return (result >= 0) ? 1 : 0;
}

int64_t my_sem_wait(char *sem_id) {
	return (int64_t) sem_wait(sem_id);
}

int64_t my_sem_post(char *sem_id) {
	return (int64_t) sem_post(sem_id);
}

int64_t my_sem_close(char *sem_id) {
	return (int64_t) sem_close(sem_id);
}

int64_t my_wait(int64_t pid) {
	return (int64_t) waitpid((int) pid);
}

int64_t my_pipe_open(char *name) {
	return sys_call(100, (uint64_t) name, 0, 0, 0, 0, 0);
}

int64_t my_pipe_close(uint64_t id) {
	return sys_call(101, id, 0, 0, 0, 0, 0);
}

int64_t my_pipe_write(uint64_t id, const char *data, uint64_t size) {
	return sys_call(102, id, (uint64_t) data, size, 0, 0, 0);
}

int64_t my_pipe_read(uint64_t id, char *buffer, uint64_t size) {
	return sys_call(103, id, (uint64_t) buffer, size, 0, 0, 0);
}