# TPE2 - Sistemas Operativos

**Autores**: Federico Spivak Fontaiña, Jerónimo Lopez Vila, Pablo Omar Germano  
**Cuatrimestre**: 2do 2025

Sistema operativo con memory manager, scheduler de procesos y manejo de prioridades.

---

## Compilacion

```bash
make              # Memory Manager Simple (bloques fijos)
make buddy        # Memory Manager Buddy (potencias de 2)
make clean        # Limpia binarios
```

## Ejecucion

```bash
./run.sh          # Ejecuta en QEMU
```

Salir de QEMU: `Ctrl+Alt+Q`

---

## Comandos Disponibles

### Sistema
- `help` - Lista de comandos
- `clear` - Limpia pantalla
- `clock` - Hora actual
- `exit` - Finaliza el sistema

### Memoria
- `test_mm <bytes>` - Test de memory manager

### Procesos
- `ps` - Lista procesos activos
- `getpid` - PID del proceso actual
- `test_processes <n>` - Test de creacion/destruccion de procesos

---

## Arquitectura

- **Video Driver**: Modo texto VGA 80x25
- **Memory Manager**: Implementaciones Simple (bitmap) y Buddy (binary tree)
- **Scheduler**: Round Robin con prioridades

---