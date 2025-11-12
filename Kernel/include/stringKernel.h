#ifndef STRING_KERNEL_H
#define STRING_KERNEL_H

#include <stddef.h>
#include <stdint.h>

/* Compara dos strings lexicográficamente.
   Retorna 0 si son iguales, <0 si s1<s2, >0 si s1>s2 */
int kstrcmp(const char *s1, const char *s2);

/* Copia hasta n-1 caracteres de src a dest y garantiza '\0' */
char *kstrncpy(char *dest, const char *src, size_t n);

/* Devuelve la longitud (sin incluir el terminador '\0') */
size_t kstrlen(const char *s);

#endif
