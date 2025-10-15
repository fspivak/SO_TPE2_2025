#ifndef _TIME_H_
#define _TIME_H_
#include <stdint.h>

#define SECONDS_TO_TICKS 18

void timer_handler();
int ticks_elapsed();
int seconds_elapsed();
void check_5seconds();
void sleep(int seconds);
uint64_t getMiSe();
/**
 * @brief Configura el timer hardware para generar interrupciones cada ~55ms
 *
 * Configura el Programmable Interval Timer (PIT) para generar interrupciones
 * cada 18 ticks, lo que equivale a aproximadamente 55.5 milisegundos (18.2 Hz).
 */
void configure_timer();

#endif
