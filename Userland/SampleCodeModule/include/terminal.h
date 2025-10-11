#include <stdint.h>

void terminal();
void help();
void clean(int ammount);
void refreshScreen();

/* Process management functions */
void list_processes();
void show_current_pid();
void run_test_process(char *args);

/* Utility functions */
void print_padded(const char *str, int width);
void print_int_padded(int value, int width);
void print_hex_padded(uint64_t value, int width);
void intToString(int value, char *buffer);
int startsWith(const char *str, const char *prefix);
