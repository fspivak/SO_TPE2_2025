# TPE2 - Sistemas Operativos

Proyecto de construccion de un nucleo de sistema operativo.

## Compilacion
```bash
make              # Simple MM
make buddy        # Buddy MM
```

## Ejecucion
```bash
./run.sh
```

## Git Hooks (Recomendado)
Instalar pre-commit hook para validacion de codigo automatico
```bash
./git-hooks/install-hooks.sh
```

Esto nos protege de:
- Commits con errores/warnings
- Auto-format code con clang-format

Leer`git-hooks/README.md` para mas detalles.

---

## Caracteristicas Implementadas

### Memory Manager Simple (Bitmap)
- Bloques fijos de 1024 bytes
- Bitmap para tracking de bloques
- First-Fit para busqueda
- Sistema de "colores" para mejor gestion de memoria
- Compatible con test_mm
- **Resultado:** Funciona con cualquier tamaño de memoria sin congelarse

### Memory Manager Buddy (Binary Tree)
- Potencias de 2 (32B minimo)
- Merge automatico de buddies
- Split dinamico de bloques
- Compatible con test_mm 
- **Resultado:** Funciona con cualquier tamaño de memoria sin congelarse

### Compilacion Alternativa
```bash
make        # Compila con MM Simple
make buddy  # Compila con MM Buddy
```

---