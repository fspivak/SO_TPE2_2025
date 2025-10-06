# TPE2 - Sistemas Operativos

Proyecto de construcción de un núcleo de sistema operativo.

## Compilación
```bash
make              # Simple MM
make buddy        # Buddy MM
```

## Ejecución
```bash
./run.sh
```

## Git Hooks (Recomendado)
Instalar pre-commit hook para validacion de codigo automatico
```bash
./git-hooks/install-hooks.sh
```

Esto nos protege de:
- ✅ Commits con errores/warnings
- ✅ Auto-format code con clang-format

Leer`git-hooks/README.md` para mas detalles.

## Estado del Proyecto

- [x] **Fase 1:** Bootloader Pure64 + Video Driver VGA 80x25
- [x] **Fase 2:** Memory Manager (Simple + Buddy)
- [ ] **Fase 3:** Procesos y Scheduler
- [ ] **Fase 4:** Sincronización (Semáforos)
- [ ] **Fase 5:** IPC - Pipes
- [ ] **Fase 6:** Comandos Userspace

**Progreso:** 33% (2 de 6 fases completas)

---

## 📝 Características Implementadas

### Memory Manager Simple (Bitmap)
- Bloques fijos de 1024 bytes
- Bitmap para tracking de bloques
- First-Fit para búsqueda
- Compatible con test_mm de cátedra
- **Resultado:** 10,000+ iteraciones sin errores

### Memory Manager Buddy (Binary Tree)
- Potencias de 2 (32B mínimo)
- Merge automático de buddies
- Split dinámico de bloques
- **Resultado:** 10,000+ iteraciones sin errores

### Compilación Alternativa
```bash
make        # Compila con MM Simple
make buddy  # Compila con MM Buddy
```

---