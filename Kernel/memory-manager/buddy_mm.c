#ifdef BUDDY_MM

#include "../include/lib.h"
#include "../include/videoDriver.h"
#include "include/memory_manager.h"

#define MIN_LEVEL 5 // Tamaño minimo de bloque: 2^5 = 32 bytes
#define MAX_ORDER 25

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
	block_t *curr_block = self->free_blocks[order];

	if (curr_block == NULL) {
		return;
	}

	// Si es el primero de la lista
	if (curr_block == block) {
		self->free_blocks[order]->status = OCCUPIED;
		self->free_blocks[order] = self->free_blocks[order]->next;
		return;
	}
	while (curr_block->next != NULL && curr_block->next != block) {
		curr_block = curr_block->next;
	}

	if (curr_block->next == NULL) {
		return;
	}

	block_t *to_remove = curr_block->next;
	curr_block->next = to_remove->next;
	to_remove->status = OCCUPIED;
}

static void split_block(MemoryManagerADT self, int8_t order) {
	block_t *block = self->free_blocks[order];
	remove_block(self, block);

	block_t *buddy_block = (block_t *) ((void *) block + (1L << (order - 1)));
	create_block(self, (void *) buddy_block, order - 1);
	create_block(self, (void *) block, order - 1);
}

static block_t *merge(MemoryManagerADT self, block_t *block, block_t *buddy_block) {
	if (block == NULL || buddy_block == NULL) {
		return NULL;
	}

	block_t *left = block < buddy_block ? block : buddy_block;
	block_t *right = block < buddy_block ? buddy_block : block;
	remove_block(self, left);
	remove_block(self, right);

	left->order++;
	left->status = FREE;
	return left;
}

MemoryManagerADT memory_manager_init(void *manager_memory, void *managed_memory) {
	MemoryManagerADT mm = (MemoryManagerADT) manager_memory;

	mm->managed_memory_start = (uint64_t) managed_memory;
	uint64_t managed_size = MEMORY_END - (uint64_t) managed_memory;
	int current_size = 2;
	int level = 1;
	while (current_size < managed_size) {
		level++;
		current_size *= 2;
	}

	mm->max_order = level;
	mm->info.total_memory = managed_size;
	mm->info.used_memory = 0;
	mm->info.free_memory = managed_size;

	const char *type = "buddy";
	for (int i = 0; i < 6; i++) {
		mm->info.mm_type[i] = (i < 5) ? type[i] : '\0';
	}

	for (int i = 0; i < MAX_ORDER; i++) {
		mm->free_blocks[i] = NULL;
	}
	create_block(mm, managed_memory, level);

	return mm;
}

void *memory_alloc(MemoryManagerADT self, const uint64_t size) {
	if (size == 0 || size > self->info.total_memory) {
		return NULL;
	}

	int8_t order = 1;
	int64_t block_size = 2;
	while (block_size < size + sizeof(block_t)) {
		order++;
		block_size *= 2;
	}

	order = (MIN_LEVEL > order) ? MIN_LEVEL : order;
	if (self->free_blocks[order] == NULL) {
		uint8_t order_approx = 0;

		for (uint8_t i = order + 1; i <= self->max_order && order_approx == 0; i++) {
			if (self->free_blocks[i] != NULL) {
				order_approx = i;
			}
		}

		if (order_approx == 0) {
			// No hay bloques disponibles
			return NULL;
		}
		while (order_approx > order) {
			split_block(self, order_approx);
			order_approx--;
		}
	}

	block_t *block = self->free_blocks[order];
	if (block == NULL) {
		return NULL;
	}

	remove_block(self, block);
	block->status = OCCUPIED;

	uint64_t allocated_size = (1UL << order);
	self->info.used_memory += allocated_size;
	self->info.free_memory -= allocated_size;

	return (void *) ((uint8_t *) block + sizeof(block_t));
}

int memory_free(MemoryManagerADT self, void *ptr) {
	if (ptr == NULL || ptr < (void *) self->managed_memory_start) {
		return -1;
	}

	block_t *block = (block_t *) (ptr - sizeof(block_t));
	if (block == NULL || block->status == FREE || (uint64_t) block >= MEMORY_END) {
		return -1;
	}

	block->status = FREE;

	uint64_t block_pos = (uint64_t) ((void *) block - self->managed_memory_start);
	block_t *buddy_block = (block_t *) (self->managed_memory_start + (((uint64_t) block_pos) ^ ((1L << block->order))));

	if (buddy_block == NULL || (uint64_t) buddy_block < self->managed_memory_start ||
		(uint64_t) buddy_block >= MEMORY_END) {
		create_block(self, (void *) block, block->order);
		return 0;
	}

	while (block && buddy_block && block->order < self->max_order && buddy_block->status == FREE &&
		   buddy_block->order == block->order) {
		block = merge(self, block, buddy_block);
		if (block == NULL) {
			return -1;
		}
		block->status = FREE;

		block_pos = (uint64_t) ((void *) block - self->managed_memory_start);
		buddy_block =
			(block_t *) (block_t *) (self->managed_memory_start + (((uint64_t) block_pos) ^ ((1L << block->order))));

		if ((uint64_t) buddy_block < self->managed_memory_start || (uint64_t) buddy_block >= MEMORY_END) {
			break;
		}
	}

	if (block == NULL) {
		return -1;
	}

	create_block(self, (void *) block, block->order);

	uint64_t freed_size = (1UL << block->order);
	self->info.used_memory -= freed_size;
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
