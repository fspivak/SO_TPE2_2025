include Makefile.inc

KERNEL=kernel.bin
KERNEL_ELF=kernel.elf
SOURCES=$(wildcard *.c)
SOURCES_ASM=$(wildcard asm/*.asm)
SCHEDULER_SOURCES=$(wildcard scheduler/*.c)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
SCHEDULER_OBJECTS=$(SCHEDULER_SOURCES:.c=.o)
LOADERSRC=loader.asm
GCCFLAGS += -w

# Memory Manager configuration
ifdef BUDDY_MM
	MM_SOURCE=memory-manager/buddy_mm.c
	GCCFLAGS += -DBUDDY_MM
else
	MM_SOURCE=memory-manager/memory_manager.c
endif

MM_OBJECT=$(MM_SOURCE:.c=.o)

LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=

all: $(KERNEL) $(KERNEL_ELF)

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(MM_OBJECT) $(SCHEDULER_OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	@echo "[Linking] kernel.bin"
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(MM_OBJECT) $(SCHEDULER_OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

$(KERNEL_ELF): $(LOADEROBJECT) $(OBJECTS) $(MM_OBJECT) $(SCHEDULER_OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	@echo "[Linking] kernel.elf (with debug symbols)"
	$(LD) -T kernel-elf.ld -o $(KERNEL_ELF) $(LOADEROBJECT) $(OBJECTS) $(MM_OBJECT) $(SCHEDULER_OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	@echo "[Compiling] $<"
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	@echo "[Assembling] $<"
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT):
	@echo "[Assembling] $(LOADERSRC)"
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	@echo "[Cleaning] kernel objects"
	rm -rf asm/*.o *.o *.bin *.elf memory-manager/*.o scheduler/*.o

.PHONY: all clean
