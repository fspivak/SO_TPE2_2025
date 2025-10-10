#!/bin/bash

if [[ $1 == "debug" ]]; then
    echo "Starting QEMU in debug mode..."
    echo "Connect GDB with: gdb -ex "target remote localhost:1234""
    echo "Press Ctrl+C to stop QEMU"
    
    # Start QEMU with debug server on port 1234
    qemu-system-x86_64 -s -S -hda Image/x64BareBonesImage.qcow2 -m 512 -audiodev pa,id=speaker -machine pcspk-audiodev=speaker
else
    echo "Starting QEMU in normal mode..."
    qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 -audiodev pa,id=speaker -machine pcspk-audiodev=speaker
fi
