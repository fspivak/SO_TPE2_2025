#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/screen.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

static void print_human_size(uint64_t bytes) {
	if (bytes >= 1024 * 1024) {
		uint64_t mb = bytes / (1024 * 1024);
		print_format("%uM", (unsigned int) mb);
	}
	else if (bytes >= 1024) {
		uint64_t kb = bytes / 1024;
		print_format("%uK", (unsigned int) kb);
	}
	else {
		print_format("%u", (unsigned int) bytes);
	}
}

static void mem_main(int argc, char **argv) {
	(void) argc;
	(void) argv;
	typedef struct {
		uint64_t total_memory;
		uint64_t used_memory;
		uint64_t free_memory;
		char mm_type[16];
	} HeapState;

	HeapState state;
	memStatus(&state);

	uint64_t total_bytes = state.total_memory;
	uint64_t used_bytes = state.used_memory;
	uint64_t free_bytes = state.free_memory;

	print_format("=== Memory Status (%s) ===\n", state.mm_type);
	print_format("Total: ");
	print_human_size(total_bytes);
	print_format("\n");
	print_format("Used:  ");
	print_human_size(used_bytes);
	print_format("\n");
	print_format("Free:  ");
	print_human_size(free_bytes);
	print_format("\n");

	if (state.total_memory > 0) {
		uint64_t used_percent = (state.used_memory * 100) / state.total_memory;
		print_format("Usage: %u%%\n", (unsigned int) used_percent);
	}
}

void mem_cmd(int argc, char **argv) {
	int pid_mem = command_spawn_process("mem", mem_main, argc, argv, 1, NULL);
	if (pid_mem < 0) {
		print_format("ERROR: Failed to create process mem\n");
		return;
	}

	command_handle_child_process(pid_mem, "mem");
}