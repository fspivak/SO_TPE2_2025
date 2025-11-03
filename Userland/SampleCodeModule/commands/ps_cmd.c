#include "../include/commands.h"
#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/screen.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

static void ps_main(int argc, char **argv) {
	(void) argc; // No usamos (convencion)
	(void) argv; // No usamos (convencion)
	typedef struct {
		int pid;
		char name[32];
		uint8_t priority;
		uint64_t stack_base;
		uint64_t rsp;
		char state_name[16];
		uint8_t hasForeground;
	} ProcessInfo;

	// Usar malloc para evitar problemas de contexto
	ProcessInfo *processes = (ProcessInfo *) malloc(sizeof(ProcessInfo) * 64);
	if (processes == NULL) {
		print("ERROR: Failed to allocate memory for ps\n");
		return;
	}

	int count = ps(processes, 64);

	if (count <= 0) {
		print("No processes found or error occurred\n");
		free(processes);
		return;
	}

	print("Active Processes:\n");
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
	print("\n");

	free(processes);
}

void ps_cmd(int argc, char **argv) {
	int pid_ps = create_process("ps", ps_main, argc, argv, 4);
	if (pid_ps < 0) {
		print("ERROR: Failed to create process ps\n");
		return;
	}

	waitpid(pid_ps);
}