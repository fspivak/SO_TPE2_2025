#ifndef SYSCALL_IDS_H
#define SYSCALL_IDS_H

/* Syscalls - Basicas */
#define SYS_WRITE 48
#define SYS_READ 49
#define SYS_MALLOC 50
#define SYS_FREE 51
#define SYS_MEM_STATUS 52

/* Syscalls - Procesos */
#define SYS_CREATE_PROCESS 60
#define SYS_GETPID 61
#define SYS_KILL 62
#define SYS_BLOCK 63
#define SYS_UNBLOCK 64
#define SYS_NICE 65
#define SYS_YIELD 66
#define SYS_PS 67
#define SYS_EXIT 68
#define SYS_WAITPID 69

#endif
