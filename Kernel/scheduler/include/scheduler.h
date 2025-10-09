#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <stdint.h>

/* Funcion llamada desde assembly - realiza context switch */
void *scheduler(void *current_stack);

/* Inicializa el scheduler */
void init_scheduler();

/* Fuerza un context switch (llamado por yield) */
void force_switch();

#endif /* _SCHEDULER_H_ */
