// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/commands.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "../include/stringUser.h"
#include "../tests/include/syscall.h"
#include <stddef.h>

#define MAX_PIPE_ARGS 8
#define MAX_PIPE_ARG_LEN 64

static int tokenize_command(const char *command, char storage[][MAX_PIPE_ARG_LEN], char *argv[], int max_tokens) {
	if (command == NULL || storage == NULL || argv == NULL || max_tokens <= 0) {
		return 0;
	}

	int count = 0;
	const char *ptr = command;

	while (*ptr != '\0' && count < max_tokens) {
		while (*ptr == ' ') {
			ptr++;
		}

		if (*ptr == '\0') {
			break;
		}

		int len = 0;
		while (ptr[len] != '\0' && ptr[len] != ' ' && len < MAX_PIPE_ARG_LEN - 1) {
			storage[count][len] = ptr[len];
			len++;
		}
		storage[count][len] = '\0';
		argv[count] = storage[count];
		count++;

		ptr += len;
		while (*ptr != '\0' && *ptr != ' ') {
			ptr++;
		}
	}

	argv[count] = NULL;
	return count;
}

// Las funciones de comandos estan declaradas en commands.h

static void (*resolve_command_function(const char *cmd))(int, char **);
static void build_writer_config(process_io_config_t *config, int pipe_id);
static void build_reader_config(process_io_config_t *config, int pipe_id);

void pipes_cmd(char *input) {
	if (command_is_background_mode()) {
		print_format("ERROR: pipes does not support background yet\n");
		return;
	}

	char *pipe_pos = strchr(input, '|');
	if (!pipe_pos)
		return;

	*pipe_pos = '\0';
	char *cmd1 = trim(input);
	char *cmd2 = trim(pipe_pos + 1);

	// Generar nombre del pipe basado en el comando escritor
	// Formato: "pipe_" + nombre_del_comando
	char pipe_name[64];
	char argv1_storage_temp[MAX_PIPE_ARGS + 1][MAX_PIPE_ARG_LEN];
	char *argv1_tokens_temp[MAX_PIPE_ARGS + 1];
	int argc1_temp = tokenize_command(cmd1, argv1_storage_temp, argv1_tokens_temp, MAX_PIPE_ARGS);

	// Construir nombre del pipe de forma segura
	pipe_name[0] = '\0';
	strcpy(pipe_name, "pipe_");
	if (argc1_temp > 0 && argv1_tokens_temp[0] != NULL) {
		// Concatenar nombre del comando escritor
		int pos = 5; // "pipe_" tiene 5 caracteres
		const char *writer_name = argv1_tokens_temp[0];
		// Limitar longitud para evitar overflow
		int max_name_len = 63 - 5; // 63 (tamaño total) - 5 ("pipe_")
		for (int i = 0; writer_name[i] != '\0' && i < max_name_len && pos < 63; i++) {
			pipe_name[pos++] = writer_name[i];
		}
		pipe_name[pos] = '\0';
	}
	else {
		// Fallback si no se puede obtener el nombre
		strcpy(pipe_name, "pipe_terminal");
	}

	int64_t pipe_id = my_pipe_open(pipe_name);
	if (pipe_id < 0) {
		print_format("ERROR: Failed to create pipe\n");
		return;
	}

	char argv1_storage[MAX_PIPE_ARGS + 1][MAX_PIPE_ARG_LEN];
	char *argv1_tokens[MAX_PIPE_ARGS + 1];
	int argc1_base = tokenize_command(cmd1, argv1_storage, argv1_tokens, MAX_PIPE_ARGS);

	char argv2_storage[MAX_PIPE_ARGS + 1][MAX_PIPE_ARG_LEN];
	char *argv2_tokens[MAX_PIPE_ARGS + 1];
	int argc2_base = tokenize_command(cmd2, argv2_storage, argv2_tokens, MAX_PIPE_ARGS);

	if (argc1_base == 0 || argc2_base == 0) {
		print_format("ERROR: Invalid pipe commands\n");
		my_pipe_close(pipe_id);
		return;
	}

	// Resolver funciones de comandos - cualquier comando puede ser escritor o lector
	void (*writer_func)(int, char **) = resolve_command_function(argv1_tokens[0]);
	void (*reader_func)(int, char **) = resolve_command_function(argv2_tokens[0]);

	if (writer_func == NULL || reader_func == NULL) {
		print_format("ERROR: Command not found or not supported in pipe\n");
		if (writer_func == NULL) {
			print_format("  Writer command '%s' not found\n", argv1_tokens[0]);
		}
		if (reader_func == NULL) {
			print_format("  Reader command '%s' not found\n", argv2_tokens[0]);
		}
		print_format("  See 'man pipe' for available commands that can be used in pipes\n");
		my_pipe_close(pipe_id);
		return;
	}

	process_io_config_t reader_config;
	process_io_config_t writer_config;
	build_reader_config(&reader_config, (int) pipe_id);
	build_writer_config(&writer_config, (int) pipe_id);

	// Crear el LECTOR PRIMERO (como en Nahue)
	// El lector se bloquea inmediatamente esperando datos en sem_read_name (inicializado en 0)
	// Cuando el escritor escribe, hace sem_post en sem_read_name y despierta al lector
	// Esto evita race conditions de forma natural sin necesidad de yields o verificaciones complejas
	int pid_reader =
		create_process_with_io(argv2_tokens[0], reader_func, argc2_base, argv2_tokens, 128, &reader_config);
	if (pid_reader < 0) {
		print_format("Error creating reader process for pipe\n");
		my_pipe_close(pipe_id);
		return;
	}

	// Dar tiempo al lector para que se registre en el pipe antes de crear el escritor
	// Esto asegura que cuando el escritor se registre, el lector ya esta listo
	yield();

	// Crear el escritor DESPUÉS del lector
	// El escritor puede escribir inmediatamente porque write_sem está inicializado en PIPE_BUFFER_SIZE
	// Cuando escribe, hace sem_post en read_sem y despierta al lector que está bloqueado
	int pid_writer =
		create_process_foreground_with_io(argv1_tokens[0], writer_func, argc1_base, argv1_tokens, 100, &writer_config);
	if (pid_writer < 0) {
		print_format("Error creating writer process for pipe\n");
		kill(pid_reader);
		waitpid(pid_reader);
		my_pipe_close(pipe_id);
		return;
	}

	waitpid(pid_writer);
	waitpid(pid_reader);
	clear_foreground(pid_writer);
	set_foreground(getpid());

	my_pipe_close(pipe_id);
}

// Funcion generica para resolver cualquier comando por nombre
// Permite que cualquier comando pueda ser usado como escritor o lector en pipes
// Usa la misma logica que el terminal para resolver comandos
static void (*resolve_command_function(const char *cmd))(int, char **) {
	if (cmd == NULL) {
		return NULL;
	}

	// Resolver comandos de la misma forma que el terminal
	// Esto permite que cualquier comando soportado por el terminal funcione en pipes
	if (!strcmp(cmd, "cat")) {
		return cat_main;
	}
	if (!strcmp(cmd, "filter")) {
		return filter_main;
	}
	if (!strcmp(cmd, "wc")) {
		return wc_main;
	}
	if (!strcmp(cmd, "ps")) {
		return ps_main;
	}
	if (!strcmp(cmd, "help")) {
		return help_main;
	}
	if (!strcmp(cmd, "man")) {
		return man_main;
	}
	if (!strcmp(cmd, "mem")) {
		return mem_main;
	}
	// Nota: Los comandos de test (test_mm, test_process, test_sync, test_prio) no pueden usarse en pipes
	// mvar tiene mvar_main pero es static, no puede usarse en pipes
	// loop y clear tienen funciones main pero son static, no pueden usarse en pipes

	// Si el comando no se encuentra, retornar NULL
	// El terminal mostrara un error apropiado
	return NULL;
}

static void build_writer_config(process_io_config_t *config, int pipe_id) {
	if (config == NULL) {
		return;
	}
	config->stdin_type = PROCESS_IO_STDIN_INHERIT;
	config->stdin_resource = PROCESS_IO_RESOURCE_INVALID;
	config->stdout_type = PROCESS_IO_STDOUT_PIPE;
	config->stdout_resource = pipe_id;
	// stderr debe ser SCREEN para que el echo de cat funcione
	// Si es INHERIT, puede que no herede correctamente la configuracion del padre
	config->stderr_type = PROCESS_IO_STDERR_SCREEN;
	config->stderr_resource = PROCESS_IO_RESOURCE_INVALID;
}

static void build_reader_config(process_io_config_t *config, int pipe_id) {
	if (config == NULL) {
		return;
	}
	config->stdin_type = PROCESS_IO_STDIN_PIPE;
	config->stdin_resource = pipe_id;
	config->stdout_type = PROCESS_IO_STDOUT_INHERIT;
	config->stdout_resource = PROCESS_IO_RESOURCE_INVALID;
	config->stderr_type = PROCESS_IO_STDERR_INHERIT;
	config->stderr_resource = PROCESS_IO_RESOURCE_INVALID;
}
