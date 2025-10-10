#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <stdint.h>

/**
 * @brief Funcion llamada desde assembly - realiza context switch
 * @param current_stack Stack pointer del proceso actual
 * @return Stack pointer del siguiente proceso a ejecutar
 */
void *scheduler(void *current_stack);

/**
 * @brief Inicializa el scheduler y crea el proceso idle
 */
void init_scheduler();

/**
 * @brief Fuerza un context switch (llamado por yield)
 */
void force_switch();

#endif /* _SCHEDULER_H_ */
