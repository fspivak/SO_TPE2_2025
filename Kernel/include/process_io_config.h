#ifndef PROCESS_IO_CONFIG_H
#define PROCESS_IO_CONFIG_H

#include <stdint.h>

/* Tipos para stdin */
#define PROCESS_IO_STDIN_INHERIT 0U
#define PROCESS_IO_STDIN_KEYBOARD 1U
#define PROCESS_IO_STDIN_PIPE 2U

/* Tipos para stdout */
#define PROCESS_IO_STDOUT_INHERIT 0U
#define PROCESS_IO_STDOUT_SCREEN 1U
#define PROCESS_IO_STDOUT_PIPE 2U

/* Tipos para stderr */
#define PROCESS_IO_STDERR_INHERIT 0U
#define PROCESS_IO_STDERR_SCREEN 1U
#define PROCESS_IO_STDERR_PIPE 2U

typedef struct {
	uint32_t stdin_type;
	int32_t stdin_resource;
	uint32_t stdout_type;
	int32_t stdout_resource;
	uint32_t stderr_type;
	int32_t stderr_resource;
} process_io_config_t;

#define PROCESS_IO_RESOURCE_INVALID (-1)

#endif /* PROCESS_IO_CONFIG_H */
