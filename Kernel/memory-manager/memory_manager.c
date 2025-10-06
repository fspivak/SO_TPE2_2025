#ifndef BUDDY_MM

#include "include/memory_manager.h"

/* Instancia global del memory manager */
MemoryManagerADT memory_manager;

/* Tamaño de bloque fijo: 1024 bytes (2^10) */
#define BLOCK_SIZE 1024
#define MAX_BLOCKS 8192 /* Suficiente para ~8MB */

struct MemoryManagerCDT {
	uint8_t *start;		   /* Inicio de memoria administrada */
	uint32_t total_blocks; /* Total de bloques disponibles */
	uint32_t used_blocks;  /* Bloques en uso */
	uint8_t *bitmap;	   /* Bitmap: 0=libre, 1-3=usado (color) */
	HeapState info;
};

MemoryManagerADT memory_manager_init(void *manager_memory, void *managed_memory) {
	MemoryManagerADT mm = (MemoryManagerADT) manager_memory;

	/* Alinear memoria administrada */
	uint64_t managed_addr = (uint64_t) managed_memory;
	uint64_t aligned_addr = (managed_addr + 7) & ~7;

	mm->start = (uint8_t *) aligned_addr;

	/* Calcular cuantos bloques de 1024 bytes caben */
	uint64_t available = MEMORY_END - aligned_addr;
	uint64_t bitmap_size = (available / BLOCK_SIZE) / 8 + 1; /* 1 bit por bloque */

	/* Bitmap inmediatamente despues de la estructura */
	mm->bitmap = (uint8_t *) (manager_memory + sizeof(struct MemoryManagerCDT));

	/* Ajustar memoria disponible (restar bitmap) */
	mm->start = (uint8_t *) (aligned_addr + bitmap_size);
	available = MEMORY_END - (uint64_t) mm->start;
	mm->total_blocks = available / BLOCK_SIZE;

	if (mm->total_blocks > MAX_BLOCKS) {
		mm->total_blocks = MAX_BLOCKS;
	}

	mm->used_blocks = 0;

	/* Inicializar bitmap (0 = libre) */
	for (uint32_t i = 0; i < mm->total_blocks; i++) {
		mm->bitmap[i] = 0;
	}

	/* Info del heap */
	mm->info.total_memory = mm->total_blocks * BLOCK_SIZE;
	mm->info.used_memory = 0;
	mm->info.free_memory = mm->info.total_memory;

	const char *type = "simple";
	for (int i = 0; i < 6 && type[i]; i++) {
		mm->info.mm_type[i] = type[i];
	}
	mm->info.mm_type[6] = '\0';

	return mm;
}

void *memory_alloc(MemoryManagerADT self, const uint64_t size) {
	if (size == 0 || size > self->info.total_memory) {
		return NULL;
	}

	/* Calcular cuantos bloques necesitamos */
	uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	if (blocks_needed > self->total_blocks - self->used_blocks) {
		return NULL;
	}

	/* Buscar bloques contiguos libres */
	uint32_t start_block = 0;
	uint32_t free_count = 0;

	for (uint32_t i = 0; i < self->total_blocks; i++) {
		if (self->bitmap[i] == 0) /* Bloque libre */
		{
			if (free_count == 0) {
				start_block = i;
			}
			free_count++;

			if (free_count == blocks_needed) {
				/* Encontramos suficientes bloques contiguos */
				/* Marcar como usados (color 1 por simplicidad) */
				for (uint32_t j = start_block; j < start_block + blocks_needed; j++) {
					self->bitmap[j] = 1;
				}

				self->used_blocks += blocks_needed;
				self->info.used_memory += blocks_needed * BLOCK_SIZE;
				self->info.free_memory = self->info.total_memory - self->info.used_memory;

				/* Retornar puntero al primer bloque */
				return (void *) (self->start + (start_block * BLOCK_SIZE));
			}
		}
		else {
			/* Bloque ocupado, resetear contador */
			free_count = 0;
		}
	}

	/* No se encontraron suficientes bloques contiguos */
	return NULL;
}

int memory_free(MemoryManagerADT self, void *ptr) {
	if (ptr == NULL || ptr < (void *) self->start) {
		return -1;
	}

	/* Calcular indice del bloque */
	uint64_t offset = (uint64_t) ptr - (uint64_t) self->start;
	uint32_t block_index = offset / BLOCK_SIZE;

	if (block_index >= self->total_blocks || self->bitmap[block_index] == 0) {
		return -1; /* Bloque invalido o ya libre */
	}

	/* Liberar todos los bloques contiguos con el mismo color */
	uint8_t color = self->bitmap[block_index];
	uint32_t freed = 0;

	while (block_index < self->total_blocks && self->bitmap[block_index] == color) {
		self->bitmap[block_index] = 0;
		block_index++;
		freed++;
	}

	self->used_blocks -= freed;
	self->info.used_memory -= freed * BLOCK_SIZE;
	self->info.free_memory = self->info.total_memory - self->info.used_memory;

	return 0;
}

void memory_state_get(MemoryManagerADT self, HeapState *state) {
	if (state == NULL) {
		return;
	}

	state->total_memory = self->info.total_memory;
	state->used_memory = self->info.used_memory;
	state->free_memory = self->info.free_memory;

	for (size_t i = 0; i < 6; i++) {
		state->mm_type[i] = self->info.mm_type[i];
	}
}

#endif
