#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <stdint.h>

/**
 * @brief Spinlock para sincronizacion atomica sin deshabilitar interrupciones
 *
 * Este spinlock usa operaciones atomicas de GCC (__sync_lock_test_and_set)
 * para garantizar exclusion mutua sin necesidad de cli/sti.
 * Si el lock esta ocupado, hace yield() en lugar de busy-wait.
 */
typedef volatile int spinlock_t;

/**
 * @brief Inicializa un spinlock
 * @param lock Puntero al spinlock a inicializar
 */
void spinlock_init(spinlock_t *lock);

/**
 * @brief Adquiere el spinlock (bloquea si esta ocupado)
 *
 * Si el lock esta ocupado, hace yield() para ceder el CPU
 * en lugar de hacer busy-wait, respetando el modelo del kernel.
 *
 * @param lock Puntero al spinlock a adquirir
 */
void spinlock_lock(spinlock_t *lock);

/**
 * @brief Libera el spinlock
 * @param lock Puntero al spinlock a liberar
 */
void spinlock_unlock(spinlock_t *lock);

#endif /* _SPINLOCK_H_ */
