#include "../include/libasmUser.h"
#include "../include/stinUser.h"
#include "include/test_util.h"

/* Declaracion de memset (implementado en _loader.c) */
extern void *memset(void *s, int c, uint64_t n);

/* Activar para mostrar informacion detallada durante el test */
#define VERBOSE 1

#define MAX_BLOCKS 128
typedef struct MM_rq {
	void *address;
	uint32_t size;
} mm_rq;

uint64_t test_mm(uint64_t argc, char *argv[]) {
	mm_rq mm_rqs[MAX_BLOCKS];
	uint8_t rq;
	uint32_t total;
	uint64_t max_memory;

	if (argc != 1)
		return -1;

	if ((max_memory = satoi(argv[0])) <= 0)
		return -1;

#ifdef VERBOSE
	print("Test started, max_memory = ");
	printBase(max_memory, 10);
	print(" bytes\n");

	uint64_t iteration = 0;
#endif

	while (1) {
		rq = 0;
		total = 0;

		// Usa analisis dinamico de fallos para evitar loop infinito
		int consecutive_failures = 0;
		while (rq < MAX_BLOCKS && total < max_memory && consecutive_failures < 20) {
			mm_rqs[rq].size = GetUniform(max_memory - total - 1) + 1;
			mm_rqs[rq].address = malloc(mm_rqs[rq].size);

			if (mm_rqs[rq].address) {
				total += mm_rqs[rq].size;
				rq++;
				consecutive_failures = 0; // reseteo en acierto
			}
			else {
				consecutive_failures++; // incremento en fallos
			}
		}

#ifdef VERBOSE
		// Informar si se detuvo la alocacion por fallos consecutivos y ajustar memoria
		if (consecutive_failures >= 20 && total < max_memory) {
			print("  [WARNING: 20 consecutive malloc failures\n");
			print("   Allocated: ");
			printBase(total, 10);
			print(" bytes of ");
			printBase(max_memory, 10);
			print(" requested\n");
			max_memory = (total > 0) ? total : (max_memory / 2);
			print("   Adjusting max_memory to: ");
			printBase(max_memory, 10);
			print(" bytes]\n");
		}
#endif

		// Set
		uint32_t i;
		for (i = 0; i < rq; i++)
			if (mm_rqs[i].address)
				memset(mm_rqs[i].address, i, mm_rqs[i].size);

		// Check
		for (i = 0; i < rq; i++)
			if (mm_rqs[i].address)
				if (!memcheck(mm_rqs[i].address, i, mm_rqs[i].size)) {
					print("test_mm ERROR - memcheck failed at block ");
					printBase(i, 10);
					print("\n");
					return -1;
				}

		// Free
		for (i = 0; i < rq; i++)
			if (mm_rqs[i].address)
				free(mm_rqs[i].address);

#ifdef VERBOSE
		iteration++;
		if (iteration % 10 == 0) {
			print("Iter ");
			printBase(iteration, 10);
			print("\n");
		}
#endif
	}

	return 1;
}
