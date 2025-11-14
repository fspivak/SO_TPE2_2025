// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifdef BUDDY_MM

#include "../include/lib.h"
#include "../include/videoDriver.h"
#include "include/memory_manager.h"

#define MIN_LEVEL 5 // Tamaño minimo de bloque: 2^5 = 32 bytes
#define MAX_ORDER 25
#define MANAGED_HEAP_SIZE (1024 * 1024) // 1 MiB

#ifndef FREE
#define FREE 0
#endif
#ifndef OCCUPIED
#define OCCUPIED 1
#endif

MemoryManagerADT memory_manager;

typedef struct block_t {
	int8_t order;
	int8_t status;
	struct block_t *next;
} block_t;

typedef struct MemoryManagerCDT {
	int8_t max_order;
	block_t *free_blocks[MAX_ORDER];
	HeapState info;
	uint64_t managed_memory_start;
} buddy_manager;

static block_t *create_block(MemoryManagerADT self, void *address, int8_t order) {
	block_t *block = (block_t *) address;
	block->order = order;
	block->status = FREE;
	block->next = self->free_blocks[order];
	self->free_blocks[order] = block;
	return block;
}

static void remove_block(MemoryManagerADT self, block_t *block) {
	if (block == NULL) {
		return;
	}

	uint8_t order = block->order;
	block_t *curr = self->free_blocks[order];
	block_t *prev = NULL;

	while (curr) {
		if (curr == block) {
			if (prev)
				prev->next = curr->next;
			else
				self->free_blocks[order] = curr->next;
			block->next = NULL;
			block->status = OCCUPIED;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

static void split_block(MemoryManagerADT self, int8_t order) {
	block_t *block = self->free_blocks[order];
	if (!block)
		return;

	remove_block(self, block);

	int8_t new_order = order - 1;
	size_t half_size = (1UL << new_order);

	block_t *buddy_block = (block_t *) ((uint8_t *) block + half_size);

	create_block(self, (void *) block, new_order);
	create_block(self, (void *) buddy_block, new_order);
}

static block_t *merge(MemoryManagerADT self, block_t *a, block_t *b) {
	if (!a || !b) {
		return NULL;
	}

	block_t *left = (a < b) ? a : b;
	block_t *right = (a < b) ? b : a;

	/* Intentamos quitarlos si están en la lista (si no están, remove no hace nada) */
	remove_block(self, left);
	remove_block(self, right);

	left->order++;
	left->status = FREE;
	return left;
}

MemoryManagerADT memory_manager_init(void *manager_memory, void *managed_memory) {
	MemoryManagerADT mm = (MemoryManagerADT) manager_memory;
	mm->managed_memory_start = (uint64_t) managed_memory;

	/* calcular potencia de 2 máxima <= MANAGED_HEAP_SIZE */
	int level = 0;
	uint64_t heap_size = 1;
	while ((heap_size << 1) <= MANAGED_HEAP_SIZE && level + 1 < MAX_ORDER) {
		heap_size <<= 1;
		level++;
	}
	mm->max_order = level;

	/* alinear inicio del heap a 2^max_order */
	uint64_t aligned_start = (mm->managed_memory_start + (heap_size - 1)) & ~(heap_size - 1);
	mm->managed_memory_start = aligned_start;

	for (int i = 0; i < MAX_ORDER; i++)
		mm->free_blocks[i] = NULL;

	create_block(mm, (void *) aligned_start, mm->max_order);

	mm->info.total_memory = heap_size;
	mm->info.used_memory = 0;
	mm->info.free_memory = heap_size;

	const char *type = "buddy";
	for (int i = 0; i < 6; i++)
		mm->info.mm_type[i] = (i < 5) ? type[i] : '\0';

	return mm;
}

void *memory_alloc(MemoryManagerADT self, const uint64_t size) {
	if (size == 0 || size > self->info.total_memory)
		return NULL;

	uint64_t total_size = size + sizeof(block_t);
	int8_t order = MIN_LEVEL;
	uint64_t block_size = (1UL << order);

	while (block_size < total_size && order < self->max_order) {
		order++;
		block_size = (1UL << order);
	}
	if (order > self->max_order)
		return NULL;

	int8_t o = order;
	while (o <= self->max_order && self->free_blocks[o] == NULL)
		o++;
	if (o > self->max_order)
		return NULL;

	while (o > order) {
		split_block(self, o);
		o--;
	}

	block_t *block = self->free_blocks[order];
	if (!block)
		return NULL;

	remove_block(self, block);
	block->status = OCCUPIED;

	self->info.used_memory += (1UL << order);
	self->info.free_memory -= (1UL << order);

	return (void *) ((uint8_t *) block + sizeof(block_t));
}

int memory_free(MemoryManagerADT self, void *ptr) {
	if (!ptr)
		return -1;

	uint64_t start = self->managed_memory_start;
	uint64_t end = start + self->info.total_memory;

	if ((uint64_t) ptr <= start || (uint64_t) ptr > end)
		return -1;

	block_t *block = (block_t *) ((uint8_t *) ptr - sizeof(block_t));
	if (!block || block->status == FREE) {
		return -1;
	}

	int8_t original_order = block->order;
	block->status = FREE;

	// Intentamos fusionar todo lo posible (merge global)
	uint64_t offset = (uint64_t) ((uint8_t *) block - start);
	block_t *buddy = (block_t *) (start + (offset ^ (1UL << block->order)));

	while ((uint64_t) buddy >= start && (uint64_t) buddy < end && buddy->status == FREE &&
		   buddy->order == block->order) {
		block = merge(self, block, buddy);
		offset = (uint64_t) ((uint8_t *) block - start);
		buddy = (block_t *) (start + (offset ^ (1UL << block->order)));
	}

	// Reinsertamos el bloque fusionado
	create_block(self, (void *) block, block->order);

	// Ajustamos correctamente la contabilidad
	uint64_t freed_size = (1UL << original_order);
	if (self->info.used_memory >= freed_size)
		self->info.used_memory -= freed_size;
	if (self->info.free_memory + freed_size <= self->info.total_memory)
		self->info.free_memory += freed_size;

	return 0;
}

void memory_state_get(MemoryManagerADT self, HeapState *state) {
	if (state == NULL) {
		return;
	}

	state->total_memory = self->info.total_memory;
	state->used_memory = self->info.used_memory;
	state->free_memory = self->info.free_memory;

	for (size_t i = 0; i < 6 && self->info.mm_type[i]; i++) {
		state->mm_type[i] = self->info.mm_type[i];
	}
	state->mm_type[6] = '\0';
}

#endif /* BUDDY_MM */
