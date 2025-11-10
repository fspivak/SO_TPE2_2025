//////////////TODO: Ver si es totalmente necesario, creo que si xq los pipes tienen nombre para que se pueda compartir
/// entre procesos

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

char *kstrcpy(char *dest, const char *src) {
	if (!dest || !src)
		return dest;

	char *d = dest;
	while ((*d++ = *src++) != '\0')
		; // copia incluyendo el terminador
	return dest;
}

char *kstrcat(char *dest, const char *src) {
	if (!dest || !src)
		return dest;

	char *d = dest;

	// ir al final de dest
	while (*d)
		d++;

	// copiar src al final
	while ((*d++ = *src++) != '\0')
		;

	return dest;
}
