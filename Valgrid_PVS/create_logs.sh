#!/bin/bash

# ============================================================
#  SO-TPE2 - Verificación de Calidad (Valgrind + PVS-Studio)
# ============================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

CONTAINER_NAME="SO-TPE2"
DOCKER_IMAGE="agodio/itba-so-multi-platform:3.0"

LOG_DIR="./logs"
mkdir -p $LOG_DIR

echo -e "${BLUE}=== SO-TPE2 - Verificación de Calidad ===${NC}"

# ============================================================
# 1. COMPILACIÓN COMPLETA USANDO DOCKER
# ============================================================
echo -e "\n${YELLOW}1. Compilando Kernel + Userland...${NC}"

# Crear contenedor si no existe
if ! docker ps -a --format "{{.Names}}" | grep -q "^${CONTAINER_NAME}$"; then
    echo "Creando contenedor ${CONTAINER_NAME}..."
    docker run -d --name ${CONTAINER_NAME} \
        -v $(pwd):/root ${DOCKER_IMAGE} \
        tail -f /dev/null
else
    docker start ${CONTAINER_NAME} >/dev/null
fi

docker exec -it ${CONTAINER_NAME} bash -c "cd /root && ./compilatodo.sh" \
    > $LOG_DIR/build.log 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ ERROR: La compilación falló${NC}"
    exit 1
fi

echo -e "${GREEN}✔ Compilación exitosa${NC}"


# ============================================================
# 2. DETECCIÓN DE WARNINGS
# ============================================================
echo -e "\n${YELLOW}2. Analizando warnings de compilación...${NC}"

WARNING_COUNT=$(grep -i "warning" $LOG_DIR/build.log | wc -l)

if [ $WARNING_COUNT -gt 0 ]; then
    echo -e "${RED}❌ Se encontraron $WARNING_COUNT warnings:${NC}"
    grep -i "warning" $LOG_DIR/build.log
    exit 1
else
    echo -e "${GREEN}✔ Sin warnings${NC}"
fi


# ============================================================
# 3. VERIFICACIÓN DE ARCHIVOS GENERADOS
# ============================================================
echo -e "\n${YELLOW}3. Verificando binarios generados...${NC}"

FILES=(
    "Kernel/kernel.bin"
    "Userland/0000-sampleCodeModule.bin"
    "Userland/0001-sampleDataModule.bin"
)

MISSING=0
for f in "${FILES[@]}"; do
    if [ ! -f "$f" ]; then
        echo -e "${RED}❌ Falta: $f${NC}"
        MISSING=1
    else
        echo -e "${GREEN}✔ $f${NC}"
    fi
done

if [ $MISSING -eq 1 ]; then
    echo -e "${RED}❌ ERROR: Faltan binarios generados${NC}"
    exit 1
fi


# ============================================================
# 4. VALGRIND SOBRE USERLAND
# ============================================================
echo -e "\n${YELLOW}4. Ejecutando Valgrind (tests del TP2)...${NC}"

# Elegís uno de los tests. Ejemplo: test_mm
VALGRIND_TARGET="./Userland/SampleCodeModule/tests/test_mm"

if [ ! -f "$VALGRIND_TARGET" ]; then
    echo -e "${RED}❌ ERROR: No existe test para Valgrind: $VALGRIND_TARGET${NC}"
else
    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --trace-children=yes \
             $VALGRIND_TARGET \
             > $LOG_DIR/valgrind.log 2>&1

    ERRORS=$(grep "ERROR SUMMARY" $LOG_DIR/valgrind.log | awk '{print $4}')
    LEAKS=$(grep "definitely lost" $LOG_DIR/valgrind.log | awk '{gsub(",", "", $4); print $4}')

    ERRORS=${ERRORS:-0}
    LEAKS=${LEAKS:-0}

    if [ "$ERRORS" -gt 0 ] || [ "$LEAKS" -gt 0 ]; then
        echo -e "${RED}❌ Valgrind encontró problemas${NC}"
        echo "Errores: $ERRORS   Fugado: $LEAKS bytes"
        exit 1
    else
        echo -e "${GREEN}✔ Valgrind: Sin problemas${NC}"
    fi
fi


# ============================================================
# 5. PVS-STUDIO (corre en EL HOST)
# ============================================================
echo -e "\n${YELLOW}5. Ejecutando análisis PVS-Studio (host)...${NC}"

if ! command -v pvs-studio-analyzer >/dev/null; then
    echo -e "${RED}❌ ERROR: PVS-Studio no está instalado en el host${NC}"
    exit 1
fi

# Capturar build real (en host) usando el contenedor
echo -e "${BLUE}  → Capturando comandos de compilación...${NC}"
pvs-studio-analyzer trace -- ./compilatodo.sh \
    > /dev/null 2>&1

echo -e "${BLUE}  → Analizando...${NC}"
pvs-studio-analyzer analyze -o $LOG_DIR/pvs.log \
    > /dev/null 2>&1

echo -e "${BLUE}  → Generando HTML...${NC}"
plog-converter -a GA:1,2 -t fullhtml \
    $LOG_DIR/pvs.log -o $LOG_DIR/pvs-report \
    > /dev/null 2>&1

PROBLEMS=$(grep -c "V[0-9]" $LOG_DIR/pvs.log)

if [ $PROBLEMS -gt 0 ]; then
    echo -e "${YELLOW}⚠ PVS-Studio encontró $PROBLEMS problemas${NC}"
else
    echo -e "${GREEN}✔ PVS-Studio: Sin problemas${NC}"
fi


# ============================================================
# 6. RESUMEN FINAL
# ============================================================
echo -e "\n${BLUE}=== RESUMEN FINAL ===${NC}"
echo -e "${GREEN}✔ Compilación sin warnings${NC}"
echo -e "${GREEN}✔ Binarios correctos${NC}"

if [ $PROBLEMS -gt 0 ]; then
    echo -e "${YELLOW}⚠ PVS-Studio encontró problemas${NC}"
else
    echo -e "${GREEN}✔ PVS-Studio sin errores${NC}"
fi

echo -e "\n${BLUE}Reportes generados en ${LOG_DIR}:${NC}"
echo -e "  📝 build.log"
echo -e "  📝 valgrind.log"
echo -e "  📝 pvs.log"
echo -e "  🌐 pvs-report/index.html"

echo -e "\n${GREEN}🎉 Verificación completa${NC}"
