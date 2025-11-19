// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifdef BUDDY_MM

#include "../include/lib.h"
#include "../include/videoDriver.h"
#include "include/memory_manager.h"

#define MIN_LEVEL 5
#define MAX_ORDER 25

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
	uint64_t managed_memory_end;
} buddy_manager;

static block_t *create_block(MemoryManagerADT self, void *address, int8_t order) {
	if (self == NULL || address == NULL || order >= MAX_ORDER) {
		return NULL;
	}

	block_t *block = (block_t *) address;
	block->order = order;
	block->status = FREE;
	block->next = NULL;

	block_t *curr = self->free_blocks[order];
	block_t *prev = NULL;

	while (curr && curr < block) {
		prev = curr;
		curr = curr->next;
	}

	if (prev) {
		prev->next = block;
	}
	else {
		self->free_blocks[order] = block;
	}
	block->next = curr;

	return block;
}

static void remove_block(MemoryManagerADT self, block_t *block) {
	if (self == NULL || block == NULL) {
		return;
	}

	uint8_t order = block->order;
	if (order >= MAX_ORDER) {
		return;
	}

	block_t *curr = self->free_blocks[order];
	block_t *prev = NULL;

	while (curr) {
		if (curr == block) {
			if (prev)
				prev->next = curr->next;
			else
				self->free_blocks[order] = curr->next;
			block->next = NULL;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

static int split_block(MemoryManagerADT self, int8_t order) {
	if (self == NULL || order < MIN_LEVEL || order >= MAX_ORDER) {
		return -1;
	}

	block_t *block = self->free_blocks[order];
	if (!block) {
		return -1;
	}

	remove_block(self, block);

	int8_t new_order = order - 1;
	if (new_order < MIN_LEVEL) {
		create_block(self, (void *) block, order);
		return -1;
	}

	uint64_t half_size = (1UL << new_order);
	uint64_t block_addr = (uint64_t) block;
	uint64_t buddy_addr = block_addr + half_size;

	if (buddy_addr + half_size > self->managed_memory_end) {
		create_block(self, (void *) block, order);
		return -1;
	}

	if (buddy_addr < self->managed_memory_start) {
		create_block(self, (void *) block, order);
		return -1;
	}

	block_t *buddy_block = (block_t *) buddy_addr;

	create_block(self, (void *) block, new_order);
	create_block(self, (void *) buddy_block, new_order);
	return 0;
}

static block_t *merge(MemoryManagerADT self, block_t *a, block_t *b) {
	if (self == NULL || !a || !b) {
		return NULL;
	}

	if (a->order != b->order || a->order >= MAX_ORDER - 1) {
		return NULL;
	}

	block_t *left = (a < b) ? a : b;
	block_t *right = (a < b) ? b : a;

	uint64_t left_addr = (uint64_t) left;
	uint64_t block_size = (1UL << left->order);
	uint64_t expected_right = left_addr + block_size;

	if ((uint64_t) right != expected_right) {
		return NULL;
	}

	if ((uint64_t) left < self->managed_memory_start || (uint64_t) right + block_size > self->managed_memory_end) {
		return NULL;
	}

	remove_block(self, left);
	remove_block(self, right);

	left->order++;
	left->status = FREE;
	return left;
}

MemoryManagerADT memory_manager_init(void *manager_memory, void *managed_memory) {
	if (manager_memory == NULL || managed_memory == NULL) {
		return NULL;
	}

	MemoryManagerADT mm = (MemoryManagerADT) manager_memory;

	uint64_t managed_addr = (uint64_t) managed_memory;
	uint64_t aligned_addr = (managed_addr + 7) & ~7;

	uint64_t available = MEMORY_END - aligned_addr;
	if (available < (1UL << MIN_LEVEL)) {
		return NULL;
	}

	int level = MIN_LEVEL;
	uint64_t block_size = (1UL << level);
	while ((block_size << 1) <= available && level + 1 < MAX_ORDER) {
		block_size <<= 1;
		level++;
	}
	mm->max_order = level;

	uint64_t heap_size = (1UL << mm->max_order);
	uint64_t aligned_block_start = (aligned_addr + (heap_size - 1)) & ~(heap_size - 1);
	if (aligned_block_start + heap_size > MEMORY_END) {
		while (mm->max_order > MIN_LEVEL) {
			mm->max_order--;
			heap_size = (1UL << mm->max_order);
			aligned_block_start = (aligned_addr + (heap_size - 1)) & ~(heap_size - 1);
			if (aligned_block_start + heap_size <= MEMORY_END) {
				break;
			}
		}
		if (aligned_block_start + heap_size > MEMORY_END) {
			return NULL;
		}
	}

	for (int i = 0; i < MAX_ORDER; i++) {
		mm->free_blocks[i] = NULL;
	}

	create_block(mm, (void *) aligned_block_start, mm->max_order);
	mm->managed_memory_start = aligned_block_start;
	mm->managed_memory_end = aligned_block_start + heap_size;
	mm->info.total_memory = heap_size;
	mm->info.used_memory = 0;
	mm->info.free_memory = heap_size;

	const char *type = "buddy";
	for (int i = 0; i < 6 && type[i]; i++) {
		mm->info.mm_type[i] = type[i];
	}
	mm->info.mm_type[6] = '\0';

	return mm;
}

void *memory_alloc(MemoryManagerADT self, const uint64_t size) {
	if (self == NULL) {
		return NULL;
	}

	if (size == 0 || size > self->info.total_memory) {
		return NULL;
	}

	uint64_t total_size = size + sizeof(block_t);
	int8_t order = MIN_LEVEL;
	uint64_t block_size = (1UL << order);

	while (block_size < total_size && order < self->max_order) {
		order++;
		block_size = (1UL << order);
	}

	if (order > self->max_order) {
		return NULL;
	}

	int8_t o = order;
	while (o <= self->max_order && self->free_blocks[o] == NULL) {
		o++;
	}
	if (o > self->max_order) {
		return NULL;
	}

	while (o > order) {
		if (split_block(self, o) != 0) {
			return NULL;
		}
		o--;
	}

	block_t *block = self->free_blocks[order];
	if (!block) {
		return NULL;
	}

	uint64_t block_addr = (uint64_t) block;
	if (block_addr < self->managed_memory_start || block_addr + block_size > self->managed_memory_end) {
		return NULL;
	}

	remove_block(self, block);
	block->status = OCCUPIED;

	uint64_t allocated_size = (1UL << order);
	if (self->info.used_memory + allocated_size <= self->info.total_memory) {
		self->info.used_memory += allocated_size;
	}
	self->info.free_memory = self->info.total_memory - self->info.used_memory;

	void *user_ptr = (void *) ((uint8_t *) block + sizeof(block_t));
	return user_ptr;
}

int memory_free(MemoryManagerADT self, void *ptr) {
	if (self == NULL) {
		return -1;
	}

	if (ptr == NULL) {
		return -1;
	}

	uint64_t ptr_addr = (uint64_t) ptr;
	if (ptr_addr < self->managed_memory_start || ptr_addr >= self->managed_memory_end) {
		return -1;
	}

	if (ptr_addr < self->managed_memory_start + sizeof(block_t)) {
		return -1;
	}

	block_t *block = (block_t *) ((uint8_t *) ptr - sizeof(block_t));
	uint64_t block_addr = (uint64_t) block;

	if (block_addr < self->managed_memory_start || block_addr >= self->managed_memory_end) {
		return -1;
	}

	if (block->status == FREE) {
		return -1;
	}

	if (block->status != OCCUPIED) {
		return -1;
	}

	int8_t original_order = block->order;
	if (original_order < MIN_LEVEL || original_order > self->max_order) {
		return -1;
	}

	uint64_t block_size = (1UL << original_order);
	if (block_addr + block_size > self->managed_memory_end) {
		return -1;
	}

	uint64_t offset = block_addr - self->managed_memory_start;

	block->status = FREE;

	uint64_t buddy_offset = offset ^ block_size;
	block_t *buddy = (block_t *) (self->managed_memory_start + buddy_offset);

	while ((uint64_t) buddy >= self->managed_memory_start && (uint64_t) buddy < self->managed_memory_end) {
		if (buddy->status != FREE || buddy->order != block->order) {
			break;
		}

		uint64_t buddy_addr = (uint64_t) buddy;
		uint64_t buddy_block_size = (1UL << buddy->order);
		uint64_t buddy_offset_check = buddy_addr - self->managed_memory_start;

		if ((buddy_offset_check & (buddy_block_size - 1)) != 0) {
			break;
		}

		if (buddy_addr + buddy_block_size > self->managed_memory_end) {
			break;
		}

		block_t *merged = merge(self, block, buddy);
		if (merged == NULL) {
			break;
		}
		block = merged;
		offset = (uint64_t) block - self->managed_memory_start;
		block_size = (1UL << block->order);
		buddy_offset = offset ^ block_size;
		buddy = (block_t *) (self->managed_memory_start + buddy_offset);
	}

	create_block(self, (void *) block, block->order);

	uint64_t freed_size = (1UL << original_order);
	if (self->info.used_memory >= freed_size) {
		self->info.used_memory -= freed_size;
	}
	self->info.free_memory = self->info.total_memory - self->info.used_memory;

	return 0;
}

void memory_state_get(MemoryManagerADT self, HeapState *state) {
	if (self == NULL || state == NULL) {
		return;
	}

	state->total_memory = self->info.total_memory;
	state->used_memory = self->info.used_memory;
	state->free_memory = self->info.total_memory - self->info.used_memory;

	size_t i = 0;
	while (i < 15 && self->info.mm_type[i] != '\0') {
		state->mm_type[i] = self->info.mm_type[i];
		i++;
	}
	state->mm_type[i] = '\0';
}

#endif /* BUDDY_MM */
