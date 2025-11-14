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
#define SYS_CREATE_PROCESS_FOREGROUND 76
#define SYS_CREATE_PROCESS_WITH_IO 77
#define SYS_CREATE_PROCESS_FOREGROUND_WITH_IO 78

/* Syscalls - Semaforos */
#define SYS_SEM_OPEN 70
#define SYS_SEM_WAIT 71
#define SYS_SEM_POST 72
#define SYS_SEM_CLOSE 73

/* Syscalls - Foreground */
#define SYS_SET_FOREGROUND 74
#define SYS_CLEAR_FOREGROUND 75

/* Syscalls - IO */
#define SYS_GET_STDIN_TYPE 79
#define SYS_CLOSE_STDIN 80
#define SYS_CLOSE_STDIN_PID 81
#define SYS_GET_FOREGROUND_PROCESS 82
#define SYS_GET_STDOUT_TYPE 83
#define SYS_READ_INPUT 84
#define SYS_WRITE_OUTPUT 85

#endif
