#include "include/libasmUser.h"
#include "include/syscall_ids.h"
#include <stdint.h>

// Declaracion de sys_call (en asm/libasmUser.asm)
extern int64_t sys_call(uint64_t id, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5,
						uint64_t arg6);

int create_process(const char *name, void (*entry)(int, char **), int argc, char **argv, int priority) {
	return (int) sys_call(SYS_CREATE_PROCESS, (uint64_t) name, (uint64_t) entry, (uint64_t) argc, (uint64_t) argv,
						  (uint64_t) priority, 0);
}

int getpid(void) {
	return (int) sys_call(SYS_GETPID, 0, 0, 0, 0, 0, 0);
}

int kill(int pid) {
	return (int) sys_call(SYS_KILL, (uint64_t) pid, 0, 0, 0, 0, 0);
}

int block(int pid) {
	return (int) sys_call(SYS_BLOCK, (uint64_t) pid, 0, 0, 0, 0, 0);
}

int unblock(int pid) {
	return (int) sys_call(SYS_UNBLOCK, (uint64_t) pid, 0, 0, 0, 0, 0);
}

int nice(int pid, int priority) {
	return (int) sys_call(SYS_NICE, (uint64_t) pid, (uint64_t) priority, 0, 0, 0, 0);
}

void yield(void) {
	sys_call(SYS_YIELD, 0, 0, 0, 0, 0, 0);
}

int ps(void *buffer, int max_processes) {
	return (int) sys_call(SYS_PS, (uint64_t) buffer, (uint64_t) max_processes, 0, 0, 0, 0);
}

int waitpid(int pid) {
	return (int) sys_call(SYS_WAITPID, (uint64_t) pid, 0, 0, 0, 0, 0);
}

int sem_open(const char *name, uint32_t initial_value) {
	return (int) sys_call(SYS_SEM_OPEN, (uint64_t) name, (uint64_t) initial_value, 0, 0, 0, 0);
}

int sem_wait(const char *name) {
	return (int) sys_call(SYS_SEM_WAIT, (uint64_t) name, 0, 0, 0, 0, 0);
}

int sem_post(const char *name) {
	return (int) sys_call(SYS_SEM_POST, (uint64_t) name, 0, 0, 0, 0, 0);
}

int sem_close(const char *name) {
	return (int) sys_call(SYS_SEM_CLOSE, (uint64_t) name, 0, 0, 0, 0, 0);
}
