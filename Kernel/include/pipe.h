///////////////TODO: Borrar esta version sin semaforos//////////

#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define PIPE_BUFFER_SIZE 1024
#define MAX_PIPES 32

typedef struct {
	char name[32];
	char buffer[PIPE_BUFFER_SIZE];
	uint64_t readIndex;
	uint64_t writeIndex;
	uint64_t count;
	uint64_t readers;
	uint64_t writers;
	uint8_t active;
} Pipe;

void init_pipes();
int pipe_open(char *name);
int pipe_close(int id);
int pipe_write(int id, const char *data, uint64_t size);
int pipe_read(int id, char *buffer, uint64_t size);

#endif

/////////////////TODO: dejar esta version con semaforos/////////////////////////////////
/*
#ifndef PIPE_H
#define PIPE_H

#include "semaphore.h"
#include <stdint.h>

#define PIPE_BUFFER_SIZE 1024
#define MAX_PIPES 32

typedef struct Pipe {
	char name[32];
	char buffer[PIPE_BUFFER_SIZE];
	uint64_t readIndex;
	uint64_t writeIndex;
	uint64_t count;

	uint64_t readers;
	uint64_t writers;

	Semaphore *sem_mutex;
	Semaphore *sem_read;
	Semaphore *sem_write;

	uint8_t active;
} Pipe;

// /**
//  * @brief Inicializa la tabla global de pipes.
//  */
// void init_pipes();

// /**
//  * @brief Crea o abre un pipe existente por nombre.
//  * Retorna índice del pipe o -1 en error.
//  */
// int pipe_open(char *name);

// /**
//  * @brief Cierra un pipe, decrementando readers/writers.
//  * Si ambos llegan a 0, se destruye.
//  */
// int pipe_close(int id);

// /**
//  * @brief Escribe `size` bytes al pipe `id`.
//  * Retorna cantidad escrita o -1 en error.
//  */
// int pipe_write(int id, const char *data, uint64_t size);

// /**
//  * @brief Lee `size` bytes del pipe `id`.
//  * Retorna cantidad leída o -1 en error.
//  */
// int pipe_read(int id, char *buffer, uint64_t size);
/*
#endif
*/