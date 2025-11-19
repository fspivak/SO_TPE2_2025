#ifndef PIPE_H
#define PIPE_H

#include "../scheduler/include/semaphore.h"
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
	int eof;

	char sem_mutex_name[MAX_SEM_NAME];
	char sem_read_name[MAX_SEM_NAME];
	char sem_write_name[MAX_SEM_NAME];
	char sem_writer_ready_name[MAX_SEM_NAME];
	int sem_mutex_id;
	int sem_read_id;
	int sem_write_id;
	int sem_writer_ready_id;

	uint8_t active;
} Pipe;

/**
 * @brief Inicializa la tabla global de pipes
 */
void init_pipes();

/**
 * @brief Crea o abre un pipe existente por nombre
 * @param name Nombre del pipe
 * @return Indice del pipe o -1 si ocurre un error
 */
int pipe_open(char *name);

/**
 * @brief Cierra un pipe decrementando contadores de lectores y escritores
 * @param id Identificador del pipe
 * @return 0 si es exitoso, -1 si ocurre un error
 */
int pipe_close(int id);

/**
 * @brief Escribe bytes en el pipe indicado
 * @param id Identificador del pipe
 * @param data Datos a escribir
 * @param size Cantidad de bytes a escribir
 * @return Cantidad de bytes escritos o -1 si ocurre un error
 */
int pipe_write(int id, const char *data, uint64_t size);

/**
 * @brief Lee bytes desde el pipe indicado
 * @param id Identificador del pipe
 * @param buffer Buffer de destino
 * @param size Cantidad de bytes a leer
 * @return Cantidad de bytes leidos o -1 si ocurre un error
 */
int pipe_read(int id, char *buffer, uint64_t size);

/**
 * @brief Registra un lector adicional para el pipe
 * @param id Identificador del pipe
 * @return 0 si es exitoso, -1 si hay error
 */
int pipe_register_reader(int id);

/**
 * @brief Desregistra un lector del pipe
 * @param id Identificador del pipe
 * @return 0 si es exitoso, -1 si hay error
 */
int pipe_unregister_reader(int id);

/**
 * @brief Registra un escritor adicional para el pipe
 * @param id Identificador del pipe
 * @return 0 si es exitoso, -1 si hay error
 */
int pipe_register_writer(int id);

/**
 * @brief Desregistra un escritor del pipe
 * @param id Identificador del pipe
 * @return 0 si es exitoso, -1 si hay error
 */
int pipe_unregister_writer(int id);

/**
 * @brief Verifica si el pipe tiene escritores activos
 * @param id Identificador del pipe
 * @return 1 si hay escritores activos, 0 si no hay o hay error
 */
int pipe_has_writers(int id);

#define PIPE_RESOURCE_INVALID -1

#endif
