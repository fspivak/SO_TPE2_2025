#include "../include/libasmUser.h"
#include "../include/stinUser.h"
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
