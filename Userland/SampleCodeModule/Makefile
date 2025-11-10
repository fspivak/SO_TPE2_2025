include ../Makefile.inc

MODULE=0000-sampleCodeModule.bin
MODULE_ELF=0000-sampleCodeModule.elf
SOURCES=$(wildcard [^_]*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
SOURCES_TESTS=$(wildcard tests/*.c)
SOURCES_COMMANDS=$(wildcard commands/*.c)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
OBJECTS_TESTS=$(SOURCES_TESTS:.c=.o)
OBJECTS_COMMANDS=$(SOURCES_COMMANDS:.c=.o)
STATICLIBS=

all: $(MODULE) $(MODULE_ELF)

$(MODULE): $(OBJECTS) $(OBJECTS_TESTS) $(OBJECTS_COMMANDS) $(OBJECTS_ASM) $(STATICLIBS)
	$(GCC) $(GCCFLAGS) -T sampleCodeModule.ld _loader.c -o ../$(MODULE) $(OBJECTS) $(OBJECTS_TESTS) $(OBJECTS_COMMANDS) $(OBJECTS_ASM) $(STATICLIBS)

$(MODULE_ELF): $(OBJECTS) $(OBJECTS_TESTS) $(OBJECTS_COMMANDS) $(OBJECTS_ASM) $(STATICLIBS)
	$(GCC) $(GCCFLAGS) -T sampleCodeModule.ld -Wl,--oformat=elf64-x86-64 _loader.c -o ../$(MODULE_ELF) $(OBJECTS) $(OBJECTS_TESTS) $(OBJECTS_COMMANDS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -I../Shared -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf asm/*.o *.o tests/*.o commands/*.o

.PHONY: all clean print