#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/screen.h"
#include "../include/stinUser.h"
#include <stdint.h>

static void print_human_size(uint64_t bytes) {
	if (bytes >= 1024 * 1024) {
		uint64_t mb = bytes / (1024 * 1024);
		printBase(mb, 10);
		print("M");
	}
	else if (bytes >= 1024) {
		uint64_t kb = bytes / 1024;
		printBase(kb, 10);
		print("K");
	}
	else {
		printBase(bytes, 10);
	}
}

static void mem_main(int argc, char **argv) {
	(void) argc; // No usamos (convencion)
	(void) argv; // No usamos (convencion)
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

	print("=== Memory Status (");
	print(state.mm_type);
	print(") ===\n");
	print("Total: ");
	print_human_size(total_bytes);
	print("\n");
	print("Used:  ");
	print_human_size(used_bytes);
	print("\n");
	print("Free:  ");
	print_human_size(free_bytes);
	print("\n");

	if (state.total_memory > 0) {
		uint64_t used_percent = (state.used_memory * 100) / state.total_memory;
		print("Usage: ");
		printBase(used_percent, 10);
		print("%\n");
	}

	exit();
}

void mem_cmd(int argc, char **argv) {
	int pid_mem = create_process("mem", mem_main, argc, argv, 4);
	if (pid_mem < 0) {
		print("ERROR: Failed to create process mem\n");
		return;
	}

	waitpid(pid_mem);
}