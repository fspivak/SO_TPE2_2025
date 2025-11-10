#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../tests/include/test_util.h"
#include <stddef.h>

#define LOOP_DEFAULT_INTERVAL_SECONDS 8
static char loop_default_interval_str[] = "8";

static int resolve_interval(int argc, char **argv) {
	if (argc <= 0 || argv == NULL || argv[0] == NULL) {
		return LOOP_DEFAULT_INTERVAL_SECONDS;
	}

	int value = satoi(argv[0]);
	if (value <= 0) {
		return LOOP_DEFAULT_INTERVAL_SECONDS;
	}
	return value;
}

static void loop_main(int argc, char **argv) {
	int pid = getpid();
	int interval = resolve_interval(argc, argv);
	if (interval <= 0) {
		interval = LOOP_DEFAULT_INTERVAL_SECONDS;
	}

	while (1) {
		print_format("Hello from loop process (PID: %d)\n", pid);
		sleepUser(interval * 1000);
	}
}

void loop_cmd(int argc, char **argv) {
	char *interval_arg = loop_default_interval_str;
	int interval_value = LOOP_DEFAULT_INTERVAL_SECONDS;

	if (argc > 1 && argv != NULL && argv[1] != NULL) {
		int parsed_interval = validate_non_negative_int("loop", "interval", argc, argv, 1);
		if (parsed_interval < 0) {
			return;
		}
		if (parsed_interval == 0) {
			print_format("ERROR: interval must be at least 1 second\n");
			return;
		}
		interval_arg = argv[1];
		interval_value = parsed_interval;
	}

	char *loop_args[] = {interval_arg, NULL};

	int pid_loop = command_spawn_process("loop", loop_main, 1, loop_args, 200, NULL);
	if (!validate_create_process_error("loop", pid_loop)) {
		return;
	}

	print_format("Loop process started (PID: %d, interval: %d seconds)\n", pid_loop, interval_value);
	command_handle_child_process(pid_loop, "loop");
}
