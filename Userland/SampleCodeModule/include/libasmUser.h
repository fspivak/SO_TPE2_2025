#include <stdint.h>

void write(int fd, char * str, int leng, int color, int bg);

void read(int fd, char * buffer, int count);

void sleep(int seg);

void zoom(int specifier);

void draw(int color, int x, int y);

void screenDetails(int * width,int * height);

void setCursor(int x, int y);

void clearScreen();

void *malloc(uint64_t size);

int free(void *ptr);

void memStatus(void *state);

void getClock(char * str);

void playSound(int index);

void getMiliSecs(uint64_t * milis);

void getcharNL(char * charac);

void impRegs();

void exit(void);

/*uint64_t* devolverRegistros();*/

void rompeOpcode();