#include "include/stringUser.h"
#include <stddef.h>
#include <stdint.h>

int strlen(char *string) {
	int count = 0;
	while (string[count] != 0) {
		count++;
	}
	return count + 1;
}

/* Compara dos strings lexicograficamente */
int strcmp(const char *str1, const char *str2) {
	while (*str1 && *str2) {
		if (*str1 != *str2) {
			return *str1 - *str2; /* Devuelve la diferencia de los caracteres */
		}
		str1++;
		str2++;
	}
	return *str1 - *str2; /* Si uno de los strings ha terminado */
}

void strcpy(char *str1, char *str2) {
	while (*str1 != '\0') {
		*str2 = *str1;
		str1++;
		str2++;
	}
	*str2 = 0;
}

char *strchr(const char *str, int c) {
	if (str == NULL)
		return NULL;

	while (*str) {
		if (*str == (char) c)
			return (char *) str;
		str++;
	}

	// Si no se encontró el carácter buscado
	return NULL;
}

char *trim(char *str) {
	while (*str == ' ' || *str == '\t')
		str++;
	char *end = str + strlen(str) - 1;
	while (end > str && (*end == ' ' || *end == '\t'))
		*end-- = '\0';
	return str;
}

char *strstr(const char *haystack, const char *needle) {
	if (!*needle)
		return (char *) haystack;
	for (; *haystack; haystack++) {
		const char *h = haystack;
		const char *n = needle;
		while (*h && *n && *h == *n) {
			h++;
			n++;
		}
		if (!*n)
			return (char *) haystack;
	}
	return NULL;
}

char *first_token(const char *str, char *dest, int max_len) {
	if (str == NULL || dest == NULL)
		return NULL;

	int i = 0;
	while (str[i] != '\0' && str[i] != ' ' && i < max_len - 1) {
		dest[i] = str[i];
		i++;
	}
	dest[i] = '\0';
	return dest;
}
