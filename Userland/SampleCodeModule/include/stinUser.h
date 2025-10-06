#include <stdint.h>

char getchar();

void putchar(char carac);

void printHexa(uint64_t value);

void print(char *string);

void printColor(char *string, int color, int bg);

void putCharColor(char carac, int color, int bg);

void printBase(uint64_t value, uint32_t base);

void zoomIn();

void zoomOut();

void imprimirRegistros();

void printRegistros(uint64_t *registros);

void sleepUser(int segs);

void clock();

void printClock(char *str);

void sound(int index);

char getcharNonLoop();

uint64_t getMS();

int numeroAleatorioEntre(int min, int max, uint64_t *seed);

unsigned int generarNumeroAleatorio(uint64_t *seed);

uint64_t getMS();