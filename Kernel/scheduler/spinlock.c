// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/spinlock.h"
#include <stddef.h>
extern void force_context_switch();

void spinlock_init(spinlock_t *lock) {
	if (lock == NULL) {
		return;
	}
	*lock = 0;
}

void spinlock_lock(spinlock_t *lock) {
	if (lock == NULL) {
		return;
	}

	while (__sync_lock_test_and_set(lock, 1) != 0) {
		force_context_switch();
	}
}

void spinlock_unlock(spinlock_t *lock) {
	if (lock == NULL) {
		return;
	}

	__sync_lock_release(lock);
}
