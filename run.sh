#!/bin/bash

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # Sin color

if [[ $1 == "debug" || $1 == "gdb" ]]; then
    echo -e "${BLUE}Starting QEMU in debug mode...${NC}"
    echo -e "${YELLOW}QEMU will freeze CPU on startup (waiting for GDB connection)${NC}"
    echo -e "${GREEN}Connect GDB with:${NC}"
    echo -e "  ${YELLOW}gdb${NC}"
    echo -e "  ${YELLOW}(gdb) target remote localhost:1234${NC}"
    echo -e "  ${YELLOW}(gdb) add-symbol-file Kernel/kernel.elf 0x100000${NC}"
    echo -e "  ${YELLOW}(gdb) add-symbol-file Userland/0000-sampleCodeModule.elf 0x400000${NC}"
    echo -e "  ${YELLOW}(gdb) c${NC}"
    echo -e "${RED}Press Ctrl+Alt+Q to kill QEMU${NC}"
    echo ""
    
    # Start QEMU with debug server on port 1234
    qemu-system-x86_64 -s -S -hda Image/x64BareBonesImage.qcow2 -m 512
elif [[ $1 == "debug-int" ]]; then
    echo -e "${BLUE}Starting QEMU in debug mode with interrupt logging...${NC}"
    echo -e "${YELLOW}QEMU will log all interrupts/exceptions${NC}"
    echo -e "${GREEN}Connect GDB with:${NC}"
    echo -e "  ${YELLOW}gdb${NC}"
    echo -e "  ${YELLOW}(gdb) target remote localhost:1234${NC}"
    echo -e "  ${YELLOW}(gdb) add-symbol-file Kernel/kernel.elf 0x100000${NC}"
    echo -e "  ${YELLOW}(gdb) add-symbol-file Userland/0000-sampleCodeModule.elf 0x400000${NC}"
    echo -e "  ${YELLOW}(gdb) c${NC}"
    echo -e "${RED}Press Ctrl+Alt+Q to kill QEMU${NC}"
    echo ""
    
    # Start QEMU with debug server and interrupt logging
    qemu-system-x86_64 -s -S -hda Image/x64BareBonesImage.qcow2 -m 512 -d int 2>&1 | grep "v=" &
    echo -e "${BLUE}QEMU started in background with interrupt logging${NC}"
    echo -e "${YELLOW}Interrupts are being logged to stderr${NC}"
else
    echo -e "${GREEN}Starting QEMU in normal mode...${NC}"
    echo -e "${RED}Press Ctrl+Alt+Q to kill QEMU${NC}"
    qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512
fi
