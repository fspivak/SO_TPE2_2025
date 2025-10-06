#include "include/test_util.h"
#include "../include/libasmUser.h"
#include "../include/stinUser.h"

/* Declaracion de memset (implementado en _loader.c) */
extern void *memset(void *s, int c, uint64_t n);

#define MAX_BLOCKS 128

typedef struct MM_rq {
  void *address;
  uint32_t size;
} mm_rq;

/* Test del Memory Manager - Ciclo infinito de malloc/free */
void test_mm(uint64_t max_memory) {

  mm_rq mm_rqs[MAX_BLOCKS];
  uint8_t rq;
  uint32_t total;

  if (max_memory <= 0) {
    print("test_mm: ERROR - max_memory debe ser > 0\n");
    return;
  }

  print("=== TEST MEMORY MANAGER ===\n");
  print("Parametro: ");
  printBase(max_memory, 10);
  print(" bytes\n");
  print("Modo VERBOSE activado\n\n");

  uint64_t iteration = 0;

  while (1) {
    rq = 0;
    total = 0;

    /* DEBUG: Inicio de iteracion */
    if (iteration % 10 == 0) {
      print("Iteracion ");
      printBase(iteration, 10);
      print(" - Solicitando bloques...\n");
    }

    /* Pedir tantos bloques como sea posible */
    while (rq < MAX_BLOCKS && total < max_memory) {
      mm_rqs[rq].size = GetUniform(max_memory - total - 1) + 1;
      
      /* DEBUG: Primer malloc */
      if (iteration == 0 && rq == 0) {
        print("DEBUG: Primer malloc, size=");
        printBase(mm_rqs[rq].size, 10);
        print("\n");
      }
      
      mm_rqs[rq].address = malloc(mm_rqs[rq].size);
      
      /* DEBUG: Resultado del primer malloc */
      if (iteration == 0 && rq == 0) {
        if (mm_rqs[rq].address) {
          print("DEBUG: malloc OK, addr=0x");
          printBase((uint64_t)mm_rqs[rq].address, 16);
          print("\n");
        } else {
          print("DEBUG: malloc retorno NULL!\n");
        }
      }

      if (mm_rqs[rq].address) {
        total += mm_rqs[rq].size;
        rq++;
      } else {
        /* Si malloc falla, salir del loop */
        break;
      }
    }
    
    /* DEBUG: Bloques obtenidos */
    if (iteration == 0) {
      print("DEBUG: Bloques obtenidos = ");
      printBase(rq, 10);
      print("\n");
    }


    /* DEBUG: Antes de memset */
    if (iteration == 0) {
      print("DEBUG: Iniciando memset...\n");
    }

    /* Setear cada bloque con un valor unico */
    uint32_t i;
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        memset(mm_rqs[i].address, i, mm_rqs[i].size);
    
    /* DEBUG: Despues de memset */
    if (iteration == 0) {
      print("DEBUG: memset completado OK\n");
    }

    /* Verificar que los bloques no se solapan */
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        if (!memcheck(mm_rqs[i].address, i, mm_rqs[i].size)) {
          print("test_mm ERROR: Bloques solapados!\n");
          print("Bloque ");
          printBase(i, 10);
          print(" corrupto\n");
          return;
        }

    /* Liberar todos los bloques */
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        free(mm_rqs[i].address);

    iteration++;
  }
}

