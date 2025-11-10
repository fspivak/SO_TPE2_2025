#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>

static int background_mode_flag = 0;
static int pending_background_pid = -1;
static const char *pending_background_name = NULL;

void command_set_background_mode(int enabled) {
	background_mode_flag = (enabled != 0);
}

int command_is_background_mode(void) {
	return background_mode_flag;
}

void command_reset_background_mode(void) {
	background_mode_flag = 0;
}

int command_pop_background_notification(int *pid, const char **name) {
	if (pending_background_pid < 0 || pending_background_name == NULL) {
		return 0;
	}

	if (pid != NULL) {
		*pid = pending_background_pid;
	}

	if (name != NULL) {
		*name = pending_background_name;
	}

	pending_background_pid = -1;
	pending_background_name = NULL;
	return 1;
}

int command_spawn_process(const char *name, void (*entry)(int, char **), int argc, char **argv, int priority) {
	if (command_is_background_mode()) {
		return create_process(name, entry, argc, argv, priority);
	}

	return create_process_foreground(name, entry, argc, argv, priority);
}

void command_handle_child_process(int pid, const char *name) {
	if (pid < 0) {
		return;
	}

	if (command_is_background_mode()) {
		pending_background_pid = pid;
		pending_background_name = name;
		return;
	}

	set_foreground(pid);
	yield();
	(void) waitpid(pid);
	clear_foreground(pid);
	set_foreground(getpid());
}
