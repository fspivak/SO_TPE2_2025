# TPE2 - Sistemas Operativos

**Autores**: Federico Spivak Fontaiña, Jerónimo Lopez Vila, Pablo Omar Germano  
**Cuatrimestre**: 2do 2025

Sistema operativo con memory manager, scheduler de procesos y manejo de prioridades.

---

## Requisitos del Sistema

### Opcion 1: Con Docker (Recomendado)
- Docker instalado y funcionando
- No requiere dependencias adicionales

**Setup inicial (solo la primera vez):**
```bash
# Construir la imagen Docker
docker build -t so-tpe2 .

# Crear y configurar el contenedor
docker run -d --name SO-TPE2 -v $(pwd):/root so-tpe2 tail -f /dev/null
```

### Opcion 2: Sin Docker
- `gcc` (compilador C)
- `nasm` (ensamblador)
- `make` (herramienta de build)
- `gcc-x86-64-linux-gnu` (cross-compiler)
- `binutils-x86-64-linux-gnu` (herramientas binarias)
- `qemu-system-x86_64` (emulador)
- `qemu-utils` (utilidades QEMU)

**Instalacion en Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install gcc nasm make gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu qemu-system-x86 qemu-utils
```

---

## Compilacion y Ejecucion

### Con Docker (Recomendado)
```bash
./compilatodo.sh         # Memory Manager Simple (bloques fijos)
./compilatodo.sh buddy   # Memory Manager Buddy (potencias de 2)
./run.sh                 # Ejecuta en QEMU
```

### Sin Docker
```bash
make              # Memory Manager Simple (bloques fijos)
make buddy        # Memory Manager Buddy (potencias de 2)
make clean        # Limpia binarios
./run.sh          # Ejecuta en QEMU
```

**Salir de QEMU:** `Ctrl+Alt+Q`

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

## Troubleshooting

### Error: "docker: command not found"
- Instalar Docker: https://docs.docker.com/get-docker/

### Error: "qemu-system-x86_64: command not found"
- Instalar QEMU: `sudo apt install qemu-system-x86 qemu-utils`

### Error: "gcc-x86-64-linux-gnu: command not found"
- Instalar cross-compiler: `sudo apt install gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu`

### Error: "nasm: command not found"
- Instalar NASM: `sudo apt install nasm`

### El contenedor Docker no existe
```bash
# Recrear el contenedor
docker rm -f SO-TPE2
docker run -d --name SO-TPE2 -v $(pwd):/root so-tpe2 tail -f /dev/null
```

---

## Arquitectura

- **Video Driver**: Modo texto VGA 80x25
- **Memory Manager**: Implementaciones Simple (bitmap) y Buddy (binary tree)
- **Scheduler**: Round Robin con prioridades

---