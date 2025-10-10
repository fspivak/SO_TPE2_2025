#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>
#include <stdint.h>

/* Definiciones de region de memoria administrada */
#define MEMORY_START 0x0000000000600000
#define MEMORY_END 0x0000000000800000
#define MEMORY_SIZE (MEMORY_END - MEMORY_START)

/* Estados de bloques */
#define FREE 1
#define OCCUPIED 0

/**
 * @brief Estructura con informacion del estado del heap
 */
typedef struct {
	uint64_t total_memory; /* Memoria total disponible */
	uint64_t used_memory;  /* Memoria actualmente en uso */
	uint64_t free_memory;  /* Memoria libre disponible */
	char mm_type[16];	   /* Tipo de MM: "simple" o "buddy" */
} HeapState;

/**
 * @brief Tipo opaco para el Memory Manager
 */
typedef struct MemoryManagerCDT *MemoryManagerADT;

/**
 * @brief Inicializa el memory manager
 *
 * @param manager_memory Puntero a region donde se guardaran las estructuras del MM
 * @param managed_memory Puntero al inicio de la memoria a administrar
 * @return Instancia inicializada del memory manager
 */
MemoryManagerADT memory_manager_init(void *manager_memory, void *managed_memory);

/**
 * @brief Reserva un bloque de memoria del tamaño especificado
 *
 * @param self Instancia del memory manager
 * @param size Cantidad de bytes a reservar (debe ser > 0)
 * @return Puntero al bloque reservado, o NULL si falla
 */
void *memory_alloc(MemoryManagerADT self, const uint64_t size);

/**
 * @brief Libera un bloque de memoria previamente reservado
 *
 * @param self Instancia del memory manager
 * @param ptr Puntero al bloque a liberar
 * @return 0 si exitoso, -1 si error
 */
int memory_free(MemoryManagerADT self, void *ptr);

/**
 * @brief Obtiene el estado actual de la memoria
 *
 * @param self Instancia del memory manager
 * @param state Puntero a estructura HeapState que se llenara con la info
 */
void memory_state_get(MemoryManagerADT self, HeapState *state);

/* Variable global del memory manager usada en todo el kernel */
extern MemoryManagerADT memory_manager;

#endif
