#ifndef LIB_H
#define LIB_H

#include <stdint.h>

void * memset(void * destination, int32_t character, uint64_t length);
void * memcpy(void * destination, const void * source, uint64_t length);

uint64_t getSeconds();
uint64_t getMinutes();
uint64_t getHours();
uint64_t getDoW();
uint64_t getMonth();
uint64_t getYear();
char returnKBOutputInterrupt();
char *cpuVendor(char *result);
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

#endif