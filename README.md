# TPE2 - Sistemas Operativos

**Comision**: S - Grupo 14  
**Autores**:  
- Federico Spivak Fontaiña (60538)  
- Jerónimo Lopez Vila (65778)  
- Pablo Omar Germano (62837)  
**Cuatrimestre**: 2do 2025

---

## Requisitos del Sistema

### Opcion 1: Con Docker (Recomendado)
- Docker instalado y funcionando
- No requiere dependencias adicionales

**Setup inicial (solo la primera vez):**
```bash
# Descargar la imagen Docker oficial
docker pull agodio/itba-so-multi-platform:3.0

# Crear y configurar el contenedor
docker run -d --name SO-TPE2 -v $(pwd):/root agodio/itba-so-multi-platform:3.0 tail -f /dev/null
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


## Instrucciones de Replicación


### Comandos del Sistema

#### Comandos Básicos
- **`help`**: Muestra lista de comandos disponibles y tests provistos por la cátedra.
- **`clear`**: Limpia la pantalla.
- **`clock`**: Muestra la hora actual del sistema.
- **`exit`**: Finaliza el sistema.
- **`sh`**: Crea una nueva shell anidada.

#### Comandos de Entrada/Salida
- **`cat`**: Imprime en pantalla el contenido recibido por entrada estándar (stdin).
- **`wc`**: Cuenta líneas, palabras y caracteres del input recibido por stdin
- **`filter`**: Filtra las vocales del input recibido por stdin.

### Comandos de Memoria

- **`mem`**: Muestra el estado actual del memory manager: memoria total, ocupada, libre y tipo de MM utilizado.

### Comandos de Procesos

- **`ps`**: Lista todos los procesos activos con sus propiedades: nombre, ID, prioridad, stack pointer, base pointer, estado y si tiene foreground.
- **`getpid`**: Muestra el PID del proceso actual.
- **`kill <pid>`**: Termina un proceso dado su ID.
  - Parámetros: `<pid>` - ID del proceso a terminar (ejemplo: `kill 5`)
- **`nice <pid> <prio>`**: Cambia la prioridad de un proceso dado su ID y la nueva prioridad.
  - Parámetros: `<pid>` - ID del proceso, `<prio>` - Nueva prioridad (0-255, donde 0 es mayor prioridad)
  - Ejemplo: `nice 5 100`
- **`block <pid>`**: Cambia el estado de un proceso entre bloqueado y listo dado su ID.
  - Parámetros: `<pid>` - ID del proceso a bloquear/desbloquear (ejemplo: `block 5`)
- **`loop <seconds>`**: Imprime su ID con un saludo cada determinada cantidad de segundos.
  - Parámetros: `<seconds>` - Intervalo en segundos entre cada saludo (ejemplo: `loop 5`)

### Testing

#### Tests Provistos por la Cátedra

- **`test_mm <bytes>`**: Test de memory manager. Ejecuta ciclo infinito que pide y libera bloques de tamaño aleatorio, verificando que no se solapen.
  - Parámetros: `<bytes>` - Cantidad máxima de memoria a utilizar en bytes (ejemplo: `test_mm 102400`)
  - Debe ejecutarse como proceso de usuario en foreground y background

- **`test_process <n>`**: Test de creación y destrucción de procesos. Ciclo infinito que crea, bloquea, desbloquea y mata procesos dummy aleatoriamente.
  - Parámetros: `<n>` - Cantidad máxima de procesos a crear (entre 1 y 64, ejemplo: `test_process 5`)

- **`test_sync <n> <use_sem>`**: Test de sincronización con semáforos. Crea procesos que incrementan y decrementan una variable global.
  - Parámetros: `<n>` - Cantidad de incrementos/decrementos a realizar, `<use_sem>` - 1 para usar semáforos, 0 para no usarlos
  - Ejemplo: `test_sync 1000 1` (con semáforos, resultado esperado = 0)
  - Ejemplo: `test_sync 1000 0` (sin semáforos, resultado variable por race conditions)

- **`test_prio <max_value>`**: Test de planificación por prioridad. Crea 3 procesos que incrementan variables. Primero con misma prioridad, luego con prioridades diferentes.
  - Parámetros: `<max_value>` - Valor al que deben llegar las variables para finalizar (ejemplo: `test_prio 500000`)

#### Comando mvar

- **`mvar <writers> <readers>`**: Implementa el problema de múltiples lectores y escritores sobre una variable global, similar a una MVar de Haskell.
  - Parámetros: `<writers>` - Cantidad de escritores (1-16), `<readers>` - Cantidad de lectores (1-16)
  - Ejemplo: `mvar 2 2` - Inicia 2 escritores y 2 lectores
  - El proceso principal termina inmediatamente después de crear los lectores y escritores
  - Cada escritor escribe valores únicos ('A', 'B', 'C', etc.)
  - Cada lector imprime valores con identificador de color único

### Caracteres Especiales

#### Pipes (`|`)
Permite conectar 2 procesos mediante un pipe unidireccional. El símbolo `|` separa dos comandos, donde la salida estándar del primero se conecta a la entrada estándar del segundo.

**Ejemplos:**
```bash
cat | wc          # cat lee de stdin y envía a wc que cuenta líneas
filter | wc       # filter filtra vocales y envía resultado a wc
cat | filter      # cat lee de stdin y envía a filter que filtra vocales
```

**Nota**: El sistema soporta conectar exactamente 2 procesos con pipes (no se soporta `p1 | p2 | p3`).

#### Comandos en Background (`&`)
Permite ejecutar un proceso en background agregando el símbolo `&` al final del comando. El shell no cede el foreground al proceso, permitiendo continuar ejecutando otros comandos.

**Ejemplo:**
```bash
loop 2 &                # Ejecuta loop en background
test_process 5 &        # Ejecuta test_process en background
test_mm 102400 &        # Ejecuta test_mm en background
```

### Atajos de Teclado

#### Ctrl+C
Interrumpe y mata el proceso que está en foreground. El sistema limpia correctamente los recursos del proceso (libera stack, remueve de tabla de procesos).

**Uso**: Presionar `Ctrl+C` mientras un proceso está ejecutándose en foreground.

#### Ctrl+D
Envía señal de End of File (EOF) a la entrada estándar. Los comandos que leen de stdin (como `cat`, `wc`, `filter`) terminan su lectura al recibir EOF.

**Uso**: Presionar `Ctrl+D` durante la lectura de entrada estándar.

---

### Análisis Estático con PVS-Studio

Para complementar las pruebas dinámicas del proyecto, se integró PVS-Studio como herramienta de análisis estático.
El objetivo es detectar errores comunes en C como:

  - uso de punteros sin verificar

  - desbordes de memoria potenciales

  - conversiones peligrosas de tipos

  - alineación incorrecta de estructuras

  - operaciones UB (Undefined Behavior)

  - código muerto o no alcanzable

  - condiciones lógicas incorrectas
    

El análisis se ejecuta automáticamente mediante el script:

```bash
./Valgrid_PVS/run_quality.sh 
```

Este script:

  - Recompila el proyecto utilizando: 
`pvs-studio-analyzer trace -- make`  para capturar todas las invocaciones reales de compilación.

  - Ejecuta el analizador con: 
`pvs-studio-analyzer analyze -f strace_out -o logs/pvs.log`

  - Genera un informe HTML completo mediante:
`plog-converter -t fullhtml logs/pvs.log -o logs/pvs/pvs_report.html`

Warnings externos: 

El bootloader entregado por la catedra de "Arquitectura de Computadoras" provoca los siguientes warnings al correr el pvs:
  - " `The 'disk' pointer was utilized before it was verified against nullptr. Check lines: 554, 577` "
    
  - " `The pointer 'dir_copy' is cast to a more strictly aligned pointer type.`"
    
  - " `The pointer 'Directory' is cast to a more strictly aligned pointer type.`" 

---

### Integración Continua

El proyecto incluye un GitHub Action configurado para ejecutarse automáticamente en cada commit. Este workflow:

  - Compila el proyecto completo utilizando la imagen Docker oficial
  - Verifica que la compilación no genere errores
  - Verifica que la compilación no genere warnings
  - Falla el commit si se detectan errores o warnings durante la compilación

Este mecanismo garantiza que el código en el repositorio siempre compile correctamente y sin warnings, cumpliendo con los requisitos del enunciado del TPE.

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
docker run -d --name SO-TPE2 -v $(pwd):/root agodio/itba-so-multi-platform:3.0 tail -f /dev/null
```

### La imagen Docker no existe
```bash
# Descargar la imagen oficial
docker pull agodio/itba-so-multi-platform:3.0
```

---

## Arquitectura

### Componentes Principales

- **Video Driver**: Modo texto VGA 80x25

- **Memory Manager**: 
  - **Simple**: Sistema de bloques fijos de 1024 bytes con bitmap para tracking. Búsqueda First-Fit de bloques contiguos.
  - **Buddy System**: Sistema de bloques en potencias de 2 (mínimo 32 bytes) con merge automático de bloques adyacentes y split dinámico.
  - Ambos comparten interfaz transparente y se seleccionan en tiempo de compilación.

- **Scheduler**: Round Robin con prioridades dinámicas (0-255). Context switching implementado en assembly con guardado/restauración completa de registros.

- **Procesos**: Sistema de multitasking preemptivo con máximo 64 procesos simultáneos.

- **Sincronización**: Semáforos con identificadores por nombre, operaciones atómicas y bloqueo de procesos sin busy waiting.

- **IPC**: Pipes unidireccionales con lectura/escritura bloqueante y transparencia para procesos (pueden leer/escribir de pipe o terminal sin modificar código).

- **Separación Kernel/Userland**: Comunicación exclusiva mediante system calls (int 0x80). Validación de parámetros en kernel para todas las syscalls.

---

## Limitaciones

### Limitaciones del Sistema

1. **Pipes**: El sistema soporta conectar exactamente 2 procesos mediante pipes. No se soporta la conexión de más de 2 procesos (por ejemplo, `p1 | p2 | p3` no está soportado).

2. **Procesos**: El sistema tiene un límite máximo de 64 procesos simultáneos.

3. **Memory Manager**: 
   - MM Simple: Bloques fijos de 1024 bytes, máximo 8192 bloques
   - Buddy System: Bloques mínimos de 32 bytes, en potencias de 2

4. **Prioridades**: Rango de 0 a 255, donde 0 es la mayor prioridad.

5. **Pipes y Semáforos**: Los identificadores por nombre tienen un límite de longitud (32 caracteres).

6. **mvar**: Máximo 16 escritores y 16 lectores simultáneos.

### Limitaciones Técnicas

1. **Modo de Video**: Solo modo texto VGA 80x25. No se soporta modo gráfico.

2. **Memoria Administrada**: Región fija de memoria entre `0x600000` y `0x800000` (~2MB).

3. **Stack por Proceso**: Tamaño fijo de 4KB por proceso.

4. **Compilación**: Requiere imagen Docker `agodio/itba-so-multi-platform:3.0`.

---

## Citas de Fragmentos de Código / Uso de IA

### Código de Referencia

El proyecto utiliza como referencia el código base proporcionado por la cátedra de Arquitectura de Computadoras para:
- Bootloader (Pure64 y BMFS)
- Drivers básicos de teclado y video
- Estructura inicial del kernel

### Uso de IA

Durante el desarrollo del proyecto se utilizó asistencia de IA para:
- Revisión de código y detección de bugs
- Sugerencias de implementación para algoritmos complejos 
- Documentación
- Análisis de requisitos del enunciado

**Nota**: Todo el código implementado fue desarrollado y revisado por los integrantes del grupo. La IA se utilizó únicamente como herramienta de asistencia, no para generar código directamente sin revisión.

---

