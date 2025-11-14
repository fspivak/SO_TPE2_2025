// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/stringKernel.h"

int kstrcmp(const char *s1, const char *s2) {
	if (!s1 || !s2)
		return (s1 == s2) ? 0 : (s1 ? 1 : -1);

	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return (unsigned char) *s1 - (unsigned char) *s2;
}

char *kstrncpy(char *dest, const char *src, size_t n) {
	if (!dest || !src || n == 0)
		return dest;

	size_t i = 0;
	for (; i + 1 < n && src[i] != '\0'; i++) {
		dest[i] = src[i];
	}
	dest[i] = '\0';
	return dest;
}

size_t kstrlen(const char *s) {
	size_t len = 0;
	if (!s)
		return 0;
	while (s[len] != '\0')
		len++;
	return len;
}
