#include "include/moduleLoader.h"
#include "include/videoDriver.h"
#include <stdint.h>
#include <string.h>

static void loadModule(uint8_t **module, void *targetModuleAddress);
static uint32_t readUint32(uint8_t **address);

void loadModules(void *payloadStart, void **targetModuleAddress) {
	int i;
	uint8_t *currentModule = (uint8_t *) payloadStart;
	uint32_t moduleCount = readUint32(&currentModule);

	for (i = 0; i < moduleCount; i++)
		loadModule(&currentModule, targetModuleAddress[i]);
}

static void loadModule(uint8_t **module, void *targetModuleAddress) {
	uint32_t moduleSize = readUint32(module);

	vd_print("  Will copy module at 0x");
	vd_print_hex((uint64_t) *module);
	vd_print(" to 0x");
	vd_print_hex((uint64_t) targetModuleAddress);
	vd_print(" (");
	vd_print_dec(moduleSize);
	vd_print(" bytes)");

	memcpy(targetModuleAddress, *module, moduleSize);
	*module += moduleSize;

	vd_print(" [Done]\n");
}

static uint32_t readUint32(uint8_t **address) {
	uint32_t result = *(uint32_t *) (*address);
	*address += sizeof(uint32_t);
	return result;
}
