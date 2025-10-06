#include <time.h>
#include <stdint.h>
#include <keyboard.h>
#include <lib.h>
#include <videoDriver.h>


static void int_20();
static void int_21();

void irqDispatcher(uint64_t irq) {
	if (irq==0) {
		int_20();
	}
	if(irq== 1){
		int_21();
	}
}

void int_20() {
	timer_handler();
}
void int_21(){
	char character = returnKBOutputInterrupt();
	bufferLoader(character);
}
