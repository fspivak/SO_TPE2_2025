#!/bin/bash

HEADER="// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
"

echo "=== Agregando PVS-Studio header a archivos .c ==="

# Buscar todos los .c del proyecto
FILES=$(find . -type f -name "*.c")

for file in $FILES; do
    # Verificar si el archivo ya contiene el header
    if grep -q "Dear PVS-Studio" "$file"; then
        echo "✔ Ya tiene header: $file"
        continue
    fi

    echo "➕ Añadiendo header a: $file"

    # Crear backup
    cp "$file" "$file.bak"

    # Insertar header al principio
    printf "%s\n\n" "$HEADER" | cat - "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
done

echo "=== Completado ==="
