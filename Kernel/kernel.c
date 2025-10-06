#include <stdint.h>
#include <string.h>
#include "include/lib.h"
#include "include/moduleLoader.h"
#include "include/videoDriver.h"
#include "include/stdinout.h"
#include "include/idtLoader.h"
#include "memory-manager/include/memory_manager.h"

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const sampleCodeModuleAddress = (void*)0x400000;
static void * const sampleDataModuleAddress = (void*)0x500000;

typedef int (*EntryPoint)();


void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase()
{
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary()
{
	char buffer[10];

	vd_clear_screen();  // Clear screen at boot start

	vd_print("[x64BareBones]\n");

	vd_print("CPU Vendor: ");
	vd_print(cpuVendor(buffer));
	vd_print("\n");

	vd_print("[Loading modules]\n");
	void * moduleAddresses[] = {
		sampleCodeModuleAddress,
		sampleDataModuleAddress
	};

	loadModules(&endOfKernelBinary, moduleAddresses);
	vd_print("[Done]\n\n");

	vd_print("[Initializing kernel's binary]\n");

	clearBSS(&bss, &endOfKernel - &bss);

	vd_print("  text: 0x");
	vd_print_hex((uint64_t)&text);
	vd_print("\n");
	vd_print("  rodata: 0x");
	vd_print_hex((uint64_t)&rodata);
	vd_print("\n");
	vd_print("  data: 0x");
	vd_print_hex((uint64_t)&data);
	vd_print("\n");
	vd_print("  bss: 0x");
	vd_print_hex((uint64_t)&bss);
	vd_print("\n");

	vd_print("[Done]\n\n");
	return getStackBase();
}

int main()
{	
	/* Inicializar Memory Manager */
	vd_print("[Initializing Memory Manager]\n");
	/* 
	 * Usar memoria alta para metadata del MM para evitar sobreescribir el stack.
	 * Colocamos metadata al inicio de MEMORY_START y la memoria administrable
	 * empieza 64KB después (espacio suficiente para 1024 fragmentos).
	 */
	#define MM_METADATA_SIZE (64 * 1024)  /* 64 KB para metadata */
	void *mm_metadata_start = (void *)MEMORY_START;
	void *managed_memory_start = (void *)(MEMORY_START + MM_METADATA_SIZE);
	
	memory_manager = memory_manager_init(
		mm_metadata_start,       /* Metadata del MM en memoria alta */
		managed_memory_start     /* Memoria administrable después */
	);
	vd_print("  MM Metadata: 0x");
	vd_print_hex((uint64_t)mm_metadata_start);
	vd_print(" (");
	vd_print_dec(MM_METADATA_SIZE / 1024);
	vd_print(" KB)\n");
	vd_print("  Managed memory: 0x");
	vd_print_hex((uint64_t)managed_memory_start);
	vd_print(" - 0x");
	vd_print_hex(MEMORY_END);
	vd_print(" (");
	vd_print_dec((MEMORY_END - (uint64_t)managed_memory_start) / 1024);
	vd_print(" KB)\n");
	vd_print("[Done]\n\n");
	
	/* Cargar IDT */
	load_idt();
	
	/* Saltar a userland */
	((EntryPoint)sampleCodeModuleAddress)();
	return 0;
}
