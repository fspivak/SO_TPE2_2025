// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef BUDDY_MM

#include "include/memory_manager.h"

MemoryManagerADT memory_manager;

#define BLOCK_SIZE 1024
#define MAX_BLOCKS 8192

struct MemoryManagerCDT {
	uint8_t *start;
	uint32_t total_blocks;
	uint32_t used_blocks;
	uint8_t *bitmap;
	uint8_t allocation_counter;
	HeapState info;
};

static uint8_t get_next_allocation_color(MemoryManagerADT self) {
	if (self == NULL) {
		return 1;
	}
	uint8_t current_color = self->allocation_counter;
	self->allocation_counter++;
	if (self->allocation_counter == 0) {
		self->allocation_counter = 1;
	}
	return current_color;
}

static uint32_t find_block_start(MemoryManagerADT self, uint32_t block_index) {
	if (self == NULL || block_index >= self->total_blocks) {
		return self->total_blocks;
	}

	uint8_t color = self->bitmap[block_index];
	if (color == 0) {
		return self->total_blocks;
	}

	uint32_t start = block_index;
	while (start > 0 && self->bitmap[start - 1] == color) {
		start--;
	}

	return start;
}

MemoryManagerADT memory_manager_init(void *manager_memory, void *managed_memory) {
	if (manager_memory == NULL || managed_memory == NULL) {
		return NULL;
	}

	MemoryManagerADT mm = (MemoryManagerADT) manager_memory;

	uint64_t managed_addr = (uint64_t) managed_memory;
	uint64_t aligned_addr = (managed_addr + 7) & ~7;

	mm->start = (uint8_t *) aligned_addr;
	uint64_t available = MEMORY_END - aligned_addr;
	mm->total_blocks = available / BLOCK_SIZE;

	if (mm->total_blocks > MAX_BLOCKS) {
		mm->total_blocks = MAX_BLOCKS;
	}

	uint64_t bitmap_size = mm->total_blocks;
	uint64_t manager_struct_size = sizeof(struct MemoryManagerCDT);
	uint64_t required_manager_size = manager_struct_size + bitmap_size;

	if (required_manager_size > (64 * 1024)) {
		return NULL;
	}

	mm->bitmap = (uint8_t *) (manager_memory + manager_struct_size);

	mm->used_blocks = 0;
	mm->allocation_counter = 1;

	for (uint32_t i = 0; i < mm->total_blocks; i++) {
		mm->bitmap[i] = 0;
	}
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
	if (self == NULL) {
		return NULL;
	}

	if (size == 0 || size > self->info.total_memory) {
		return NULL;
	}

	uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	if (blocks_needed > self->total_blocks - self->used_blocks) {
		return NULL;
	}
	uint32_t start_block = 0;
	uint32_t free_count = 0;

	for (uint32_t i = 0; i < self->total_blocks; i++) {
		if (self->bitmap[i] == 0) {
			if (free_count == 0) {
				start_block = i;
			}
			free_count++;

			if (free_count == blocks_needed) {
				uint8_t all_free = 1;
				for (uint32_t j = start_block; j < start_block + blocks_needed; j++) {
					if (self->bitmap[j] != 0) {
						all_free = 0;
						break;
					}
				}

				if (!all_free) {
					free_count = 0;
					continue;
				}

				if (start_block > 0 && self->bitmap[start_block - 1] != 0) {
					free_count = 0;
					continue;
				}

				if (start_block + blocks_needed < self->total_blocks &&
					self->bitmap[start_block + blocks_needed] != 0) {
					free_count = 0;
					continue;
				}

				uint8_t allocation_color = get_next_allocation_color(self);

				for (uint32_t j = start_block; j < start_block + blocks_needed; j++) {
					self->bitmap[j] = allocation_color;
				}

				self->used_blocks += blocks_needed;
				self->info.used_memory += blocks_needed * BLOCK_SIZE;
				self->info.free_memory = self->info.total_memory - self->info.used_memory;
				return (void *) (self->start + (start_block * BLOCK_SIZE));
			}
		}
		else {
			free_count = 0;
		}
	}

	return NULL;
}

int memory_free(MemoryManagerADT self, void *ptr) {
	if (self == NULL) {
		return -1;
	}

	if (ptr == NULL || ptr < (void *) self->start) {
		return -1;
	}

	uint64_t offset = (uint64_t) ptr - (uint64_t) self->start;
	uint64_t max_valid_offset = (uint64_t) self->total_blocks * BLOCK_SIZE;

	if (offset >= max_valid_offset) {
		return -1;
	}

	uint32_t block_index;
	if (offset % BLOCK_SIZE != 0) {
		block_index = offset / BLOCK_SIZE;
		uint32_t start_block = find_block_start(self, block_index);
		if (start_block >= self->total_blocks) {
			return -1;
		}
		block_index = start_block;
	}
	else {
		block_index = offset / BLOCK_SIZE;
		if (block_index > 0 && self->bitmap[block_index - 1] == self->bitmap[block_index]) {
			uint32_t start_block = find_block_start(self, block_index);
			if (start_block >= self->total_blocks) {
				return -1;
			}
			block_index = start_block;
		}
	}

	if (self->bitmap[block_index] == 0) {
		return -1;
	}

	uint8_t color = self->bitmap[block_index];

	uint32_t expected_blocks = 0;
	uint32_t check_index = block_index;
	while (check_index < self->total_blocks && self->bitmap[check_index] == color) {
		expected_blocks++;
		check_index++;
	}

	if (expected_blocks == 0) {
		return -1;
	}

	uint32_t freed = 0;
	uint32_t current_block = block_index;

	while (current_block < self->total_blocks && self->bitmap[current_block] == color && freed < expected_blocks) {
		self->bitmap[current_block] = 0;
		current_block++;
		freed++;
	}

	if (freed != expected_blocks) {
		for (uint32_t i = block_index; i < block_index + freed; i++) {
			self->bitmap[i] = color;
		}
		return -1;
	}

	self->used_blocks -= freed;
	self->info.used_memory -= freed * BLOCK_SIZE;
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

#endif
