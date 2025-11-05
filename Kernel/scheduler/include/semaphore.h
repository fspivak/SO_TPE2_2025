#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "process.h"
#include <stdint.h>

#define MAX_SEMAPHORES 64
#define MAX_SEM_NAME 32
#define SEM_NOT_FOUND -1

typedef enum { SEM_FREE, SEM_USED } SemaphoreState;

typedef struct {
	char name[MAX_SEM_NAME];
	uint32_t value;
	process_id_t waiting_processes[MAX_PROCESSES];
	uint16_t head;	// Índice del primer proceso en la cola
	uint16_t tail;	// Índice donde se insertará el próximo proceso
	uint16_t count; // Número de procesos en la cola
	SemaphoreState state;
	uint16_t users; // Contador de referencias (sem_open/sem_close)
} sem_t;

/**
 * @brief Abre o crea un semaforo con el nombre y valor inicial especificados
 * @param name Nombre del semaforo
 * @param initial_value Valor inicial del semaforo
 * @return sem_id del semaforo (>= 0) si exitoso, -1 si hay error
 */
int sem_open(const char *name, uint32_t initial_value);

/**
 * @brief Espera en el semaforo (decrementa el valor o bloquea el proceso)
 * @param name Nombre del semaforo
 * @return 0 si exitoso, -1 si hay error
 */
int sem_wait(const char *name);

/**
 * @brief Libera el semaforo (incrementa el valor o desbloquea un proceso)
 * @param name Nombre del semaforo
 * @return 0 si exitoso, -1 si hay error
 */
int sem_post(const char *name);

/**
 * @brief Cierra el semaforo (decrementa contador de referencias)
 * @param name Nombre del semaforo
 * @return 0 si exitoso, -1 si hay error
 */
int sem_close(const char *name);

/**
 * @brief Obtiene la cantidad de procesos esperando en un semaforo
 * @param name Nombre del semaforo
 * @return Cantidad de procesos esperando, -1 si hay error
 */
int sem_get_waiting_count(const char *name);

#endif
