#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

void print_padded(const char *str, int width) {
	int len = 0;
	while (str[len] != '\0' && len < 100)
		len++; // Evitar overflow

	print((char *) str);
	for (int i = len; i < width; i++) {
		print(" ");
	}
}

void print_int_padded(int value, int width) {
	char buffer[20];
	int pos = 0;

	// Convertir entero a string
	if (value == 0) {
		buffer[pos++] = '0';
	}
	else {
		int temp = value;
		if (temp < 0) {
			buffer[pos++] = '-';
			temp = -temp;
		}

		char digits[20];
		int digit_count = 0;
		while (temp > 0) {
			digits[digit_count++] = '0' + (temp % 10);
			temp /= 10;
		}

		// Invertir los dígitos
		for (int i = digit_count - 1; i >= 0; i--) {
			buffer[pos++] = digits[i];
		}
	}
	buffer[pos] = '\0';

	print_padded(buffer, width);
}

void print_hex_padded(uint64_t value, int width) {
	char buffer[20];
	const char hex_chars[] = "0123456789ABCDEF";
	int pos = 0;

	// Agregar prefijo 0x
	buffer[pos++] = '0';
	buffer[pos++] = 'x';

	// Convertir a hexadecimal (maximo 16 digitos para uint64_t)
	char hex_digits[16];
	int digit_count = 0;

	if (value == 0) {
		hex_digits[digit_count++] = '0';
	}
	else {
		uint64_t temp = value;
		while (temp > 0) {
			hex_digits[digit_count++] = hex_chars[temp % 16];
			temp /= 16;
		}
	}

	// Invertir los digitos hex
	for (int i = digit_count - 1; i >= 0; i--) {
		buffer[pos++] = hex_digits[i];
	}
	buffer[pos] = '\0';

	print_padded(buffer, width);
}

// Función para convertir un entero a string
void intToString(int value, char *buffer) {
	if (value == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	char temp[10];
	int pos = 0;
	int temp_value = value;

	if (temp_value < 0) {
		temp_value = -temp_value;
	}

	while (temp_value > 0) {
		temp[pos++] = '0' + (temp_value % 10);
		temp_value /= 10;
	}

	int buffer_pos = 0;
	if (value < 0) {
		buffer[buffer_pos++] = '-';
	}

	for (int i = pos - 1; i >= 0; i--) {
		buffer[buffer_pos++] = temp[i];
	}
	buffer[buffer_pos] = '\0';
}

// Función para verificar si un string comienza con un prefijo
int startsWith(const char *str, const char *prefix) {
	if (str == NULL || prefix == NULL) {
		return 0;
	}

	int i = 0;
	while (prefix[i] != '\0') {
		if (str[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return 1;
}

// Función para ejecutar comandos con argumentos
void execute_command_with_args(const char *buffer, const char *command_prefix, int prefix_len,
							   void (*command_func)(int, char **)) {
	if (buffer == NULL || command_prefix == NULL || command_func == NULL) {
		print("ERROR: Invalid arguments to execute_command_with_args\n");
		return;
	}

	// Contar argumentos
	int argc = 1; // El comando mismo cuenta como argumento
	int i = prefix_len;

	// Saltar el espacio después del comando
	if (buffer[i] == ' ') {
		i++;
	}

	// Contar argumentos restantes
	while (buffer[i] != '\0') {
		// Si encontramos un espacio, incrementar argc
		if (buffer[i] == ' ') {
			argc++;
			// Saltar espacios consecutivos
			while (buffer[i] == ' ') {
				i++;
			}
		}
		else {
			i++;
		}
	}

	// Si hay contenido después del comando, incrementar argc
	int buffer_len = 0;
	while (buffer[buffer_len] != '\0') {
		buffer_len++;
	}
	if (prefix_len < buffer_len && buffer[prefix_len] != ' ') {
		argc++;
	}

	// Crear array de argumentos
	static char *argv[10];			  // Máximo 10 argumentos
	static char arg_strings[10][100]; // Buffer para strings de argumentos

	// Parsear argumentos
	int arg_index = 0;
	int char_index = prefix_len;

	// Saltar el espacio después del comando
	if (buffer[char_index] == ' ') {
		char_index++;
	}

	// Primer argumento es el comando (sin el espacio final)
	static char command_name[50];
	int cmd_len = 0;
	while (command_prefix[cmd_len] != ' ' && command_prefix[cmd_len] != '\0') {
		command_name[cmd_len] = command_prefix[cmd_len];
		cmd_len++;
	}
	command_name[cmd_len] = '\0';
	argv[arg_index] = command_name;
	arg_index++;

	// Parsear argumentos restantes
	while (buffer[char_index] != '\0' && arg_index < 10) {
		// Saltar espacios iniciales
		while (buffer[char_index] == ' ') {
			char_index++;
		}

		if (buffer[char_index] == '\0') {
			break;
		}

		// Copiar argumento
		int i = 0;
		while (buffer[char_index] != ' ' && buffer[char_index] != '\0' && i < 99) {
			arg_strings[arg_index - 1][i] = buffer[char_index];
			char_index++;
			i++;
		}
		arg_strings[arg_index - 1][i] = '\0';
		argv[arg_index] = arg_strings[arg_index - 1];
		arg_index++;
	}

	// Llamar a la función del comando
	command_func(argc, argv);
}
