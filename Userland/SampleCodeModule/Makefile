include ../Makefile.inc

MODULE=0000-sampleCodeModule.bin
SOURCES=$(wildcard [^_]*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
SOURCES_TESTS=$(wildcard tests/*.c)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
OBJECTS_TESTS=$(SOURCES_TESTS:.c=.o)
STATICCLIBS=

all: $(MODULE)

$(MODULE): $(OBJECTS) $(OBJECTS_TESTS) $(OBJECTS_ASM) $(STATICLIBS)
	$(GCC) $(GCCFLAGS) -T sampleCodeModule.ld _loader.c -o ../$(MODULE) $(OBJECTS) $(OBJECTS_TESTS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf asm/*.o *.o tests/*.o

.PHONY: all clean print