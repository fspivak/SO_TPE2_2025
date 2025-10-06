#ifndef _TIME_H_
#define _TIME_H_
#include <stdint.h>

void timer_handler();
int ticks_elapsed();
int seconds_elapsed();
void check_5seconds();
void sleep(int seconds);
uint64_t getMiSe();

#endif
