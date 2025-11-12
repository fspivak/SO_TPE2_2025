#include <stdint.h>

char getchar();

void putchar(char carac);

void print_format(const char *format, ...);

void printColor(char *string, int color, int bg);

void putCharColor(char carac, int color, int bg);

void sleepUser(int segs);

void clock();

void printClock(char *str);

void sound(int index);

char getcharNonLoop();

uint64_t getMS();

int numeroAleatorioEntre(int min, int max, uint64_t *seed);

unsigned int generarNumeroAleatorio(uint64_t *seed);