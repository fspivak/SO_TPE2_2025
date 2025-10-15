#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/screen.h"
#include "../include/stinUser.h"
#include <stdint.h>

void ps_cmd(int argc, char **argv) {
	typedef struct {
		int pid;
		char name[32];
		uint8_t priority;
		uint64_t stack_base;
		uint64_t rsp;
		char state_name[16];
		uint8_t hasForeground;
	} ProcessInfo;

	ProcessInfo processes[64];
	int count = ps(processes, 64);

	if (count <= 0) {
		print("No processes found or error occurred\n");
		return;
	}

	print("\nActive Processes:\n");
	print_padded("PID", 6);
	print_padded("Name", 12);
	print_padded("Priority", 10);
	print_padded("State", 12);
	print_padded("Stack Base", 16);
	print_padded("RSP", 16);
	print("FG\n");
	print("-----------------------------------------------------------------------------\n");

	for (int i = 0; i < count; i++) {
		print_int_padded(processes[i].pid, 6);
		print_padded(processes[i].name, 12);
		print_int_padded(processes[i].priority, 10);
		print_padded(processes[i].state_name, 12);
		print_hex_padded(processes[i].stack_base, 16);
		print_hex_padded(processes[i].rsp, 16);
		print_padded(processes[i].hasForeground ? "1" : "0", 3);
		print("\n");
	}

	print("-----------------------------------------------------------------------------\n");
	print("Total processes: ");
	printBase(count, 10);
	print("\n\n");
}
