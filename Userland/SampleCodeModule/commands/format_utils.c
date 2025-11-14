// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "../include/format_utils.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include <stddef.h>
#include <stdint.h>

void print_padded(const char *str, int width) {
	int len = 0;
	while (str[len] != '\0' && len < 100)
		len++;

	print_format("%s", str);
	for (int i = len; i < width; i++) {
		print_format(" ");
	}
}

void print_int_padded(int value, int width) {
	char buffer[20];
	int pos = 0;

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

	buffer[pos++] = '0';
	buffer[pos++] = 'x';

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

	for (int i = digit_count - 1; i >= 0; i--) {
		buffer[pos++] = hex_digits[i];
	}
	buffer[pos] = '\0';

	print_padded(buffer, width);
}

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

void execute_command_with_args(const char *buffer, const char *command_prefix, int prefix_len,
							   void (*command_func)(int, char **)) {
	if (buffer == NULL || command_prefix == NULL || command_func == NULL) {
		print_format("ERROR: Invalid arguments to execute_command_with_args\n");
		return;
	}

	int argc_estimate = 1;
	int i = prefix_len;

	while (buffer[i] == ' ') {
		i++;
	}

	int has_content = 0;
	while (buffer[i] != '\0') {
		if (buffer[i] != ' ') {
			has_content = 1;
			i++;
		}
		else {
			if (has_content) {
				argc_estimate++;
				has_content = 0;
			}
			while (buffer[i] == ' ') {
				i++;
			}
		}
	}

	if (has_content) {
		argc_estimate++;
	}

	int argc = argc_estimate;

	static char *argv[10];
	static char arg_strings[10][100];

	int arg_index = 0;
	int char_index = prefix_len;

	if (buffer[char_index] == ' ') {
		char_index++;
	}

	static char command_name[50];
	int cmd_len = 0;
	while (command_prefix[cmd_len] != ' ' && command_prefix[cmd_len] != '\0') {
		command_name[cmd_len] = command_prefix[cmd_len];
		cmd_len++;
	}
	command_name[cmd_len] = '\0';
	argv[arg_index] = command_name;
	arg_index++;

	while (buffer[char_index] != '\0' && arg_index < 10) {
		while (buffer[char_index] == ' ') {
			char_index++;
		}

		if (buffer[char_index] == '\0') {
			break;
		}

		int i = 0;
		while (buffer[char_index] != ' ' && buffer[char_index] != '\0' && i < 99) {
			arg_strings[arg_index - 1][i] = buffer[char_index];
			char_index++;
			i++;
		}

		if (i > 0) {
			arg_strings[arg_index - 1][i] = '\0';
			argv[arg_index] = arg_strings[arg_index - 1];
			arg_index++;
		}
	}

	argv[arg_index] = NULL;

	argc = arg_index;

	command_func(argc, argv);
}
