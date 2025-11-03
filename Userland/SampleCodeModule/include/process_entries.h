#ifndef PROCESS_ENTRIES_H
#define PROCESS_ENTRIES_H

#include <stdint.h>

void test_process_entry(uint64_t argc, char *argv[]);
void clock_entry(uint64_t argc, char *argv[]);
void test_ab_entry(uint64_t argc, char *argv[]);
void test_mm_entry(uint64_t argc, char *argv[]);

#endif
