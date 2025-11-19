// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/idtLoader.h"
#include "include/moduleLoader.h"
#include "include/time.h"
#include "include/videoDriver.h"
#include "memory-manager/include/memory_manager.h"
#include "scheduler/include/scheduler.h"
#include <stdint.h>
#include <string.h>

/* Referencias a secciones del kernel */
extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

/* Configuracion del sistema */
static const uint64_t PageSize = 0x1000;

/* Direcciones de los modulos de userland */
static void *const sampleCodeModuleAddress = (void *) 0x400000;
static void *const sampleDataModuleAddress = (void *) 0x500000;

typedef int (*EntryPoint)();

void clearBSS(void *bssAddress, uint64_t bssSize) {
	memset(bssAddress, 0, bssSize);
}

void *getStackBase() {
	return (void *) ((uint64_t) &endOfKernel + PageSize * 8 // The size of the stack itself, 32KiB
					 - sizeof(uint64_t)						// Begin at the top of the stack
	);
}

void *initializeKernelBinary() {
	char buffer[10];

	vd_clear_screen(); // Clear screen at boot start

	void *moduleAddresses[] = {sampleCodeModuleAddress, sampleDataModuleAddress};

	loadModules(&endOfKernelBinary, moduleAddresses);

	clearBSS(&bss, &endOfKernel - &bss);

	return getStackBase();
}

/**
 * Funcion principal del kernel
 *
 * Inicializa todos los componentes del sistema operativo:
 * 1. Memory Manager con memoria alta para evitar conflictos
 * 2. IDT (Interrupt Descriptor Table)
 * 3. Scheduler para manejo de procesos
 * 4. Salta a userland para comenzar la ejecucion
 *
 * @return 0 si la inicializacion es exitosa
 */
int main() {
	/* Inicializar Memory Manager */
/*
 * Usar memoria alta para metadata del MM para evitar sobreescribir el stack.
 * Colocamos metadata al inicio de MEMORY_START y la memoria administrable
 * empieza 64KB después (espacio suficiente para 1024 fragmentos).
 */
#define MM_METADATA_SIZE (64 * 1024) /* 64 KB para metadata */
	void *mm_metadata_start = (void *) MEMORY_START;
	void *managed_memory_start = (void *) (MEMORY_START + MM_METADATA_SIZE);

	memory_manager = memory_manager_init(mm_metadata_start,	  /* Metadata del MM en memoria alta */
										 managed_memory_start /* Memoria administrable después */
	);

	/* Configurar timer hardware para interrupciones cada 18 ticks (55ms por contexto) */
	configure_timer();

	/* Inicializar Scheduler y Procesos */
	// init_scheduler() llama a init_processes() que llama a init_pipes()
	// pipes inicializados ANTES de saltar a userland
	init_scheduler();

	/* Cargar IDT (lo ejecutamos despues de init_scheduler para evitar interrupciones durante inicializacion) */
	load_idt();

	/* Saltar a userland */
	((EntryPoint) sampleCodeModuleAddress)();
	return 0;
}
