#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== SO-TPE2 — Análisis Completo (Valgrind + PVS) ===${NC}"

mkdir -p logs
mkdir -p logs/valgrind
mkdir -p logs/pvs

#############################################
# 1) BUILD
#############################################
echo -e "\n${YELLOW}▶ 1. Compilando con compilatodo.sh...${NC}"

./compilatodo.sh > logs/build_stdout.log 2> logs/build_stderr.log
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ ERROR: Falló la compilación (ver logs/build_stderr.log)${NC}"
    exit 1
fi
echo -e "${GREEN}✔ Compilación exitosa${NC}"

#############################################
# 2) VALGRIND
#############################################
echo -e "\n${YELLOW}▶ 2. Ejecutando Valgrind...${NC}"

# Asegurar que el container esté corriendo
if ! docker ps --format '{{.Names}}' | grep -q "^SO-TPE2$"; then
    echo -e "${YELLOW}⚠ El contenedor SO-TPE2 no está corriendo. Intentando arrancarlo...${NC}"
    docker start SO-TPE2 >/dev/null 2>&1
fi

# Si aun no está corriendo => error
if ! docker ps --format '{{.Names}}' | grep -q "^SO-TPE2$"; then
    echo -e "${RED}❌ ERROR: No se pudo iniciar el contenedor SO-TPE2${NC}"
    exit 1
fi

docker exec SO-TPE2 bash -c "
mkdir -p /root/logs
cd /root/Userland/SampleCodeModule/tests

valgrind --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    ./test_mm \
    > /root/logs/valgrind_test_mm.log 2>&1

valgrind --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    ./test_sync \
    > /root/logs/valgrind_test_sync.log 2>&1

valgrind --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    ./test_processes \
    > /root/logs/valgrind_test_processes.log 2>&1
"

# Copiar logs al host
docker cp SO-TPE2:/root/logs/. logs/valgrind >/dev/null 2>&1

echo -e "${GREEN}✔ Valgrind ejecutado${NC}"

#############################################
# 3) PVS-STUDIO
#############################################
echo -e "\n${YELLOW}▶ 3. Ejecutando PVS-Studio (modo strace)...${NC}"

# eliminar rastros previos
rm -f strace_out logs/pvs.log
rm -rf logs/pvs/pvs_report.html

echo -e "${BLUE}   → Generando strace_out...${NC}"

# IMPORTANTE: compilar de nuevo con wrapper para capturar comandos
pvs-studio-analyzer trace -- \
    make -j$(nproc) > logs/pvs_make.log 2>&1

# crear carpeta pvs si no existe
mkdir -p logs/pvs

echo -e "${BLUE}   → Ejecutando análisis PVS...${NC}"

# correr analizador en base al strace generado
pvs-studio-analyzer analyze \
    -f strace_out \
    -o logs/pvs.log

echo -e "${BLUE}   → Generando reporte HTML...${NC}"

plog-converter \
    -t fullhtml \
    logs/pvs.log \
    -o logs/pvs/pvs_report.html

echo -e "${GREEN}✔ PVS-Studio completado con strace_out${NC}"


#############################################
# 4) RESUMEN
#############################################
echo -e "\n${BLUE}=== ANÁLISIS COMPLETO ===${NC}"

echo -e "📄 build:"
echo -e "   → logs/build_stdout.log"
echo -e "   → logs/build_stderr.log"

echo -e "🐘 Valgrind:"
ls logs/valgrind/*.log 2>/dev/null || echo "   (no hay logs)"

echo -e "🧠 PVS-Studio:"
echo -e "   → logs/pvs.log"
echo -e "   → logs/pvs/pvs_report.html"

echo -e "\n${GREEN}🎉 Análisis completado${NC}"

# #!/bin/bash

# RED='\033[0;31m'
# GREEN='\033[0;32m'
# YELLOW='\033[1;33m'
# BLUE='\033[0;34m'
# NC='\033[0m'

# echo -e "${BLUE}=== SO-TPE2 — Análisis Completo (Valgrind + PVS) ===${NC}"

# mkdir -p logs
# mkdir -p logs/valgrind
# mkdir -p logs/pvs

# #############################################
# # 1) BUILD
# #############################################
# echo -e "\n${YELLOW}▶ 1. Compilando con compilatodo.sh...${NC}"

# ./compilatodo.sh > logs/build_stdout.log 2> logs/build_stderr.log
# if [ $? -ne 0 ]; then
#     echo -e "${RED}❌ ERROR: Falló la compilación (ver logs/build_stderr.log)${NC}"
#     exit 1
# fi
# echo -e "${GREEN}✔ Compilación exitosa${NC}"

# #############################################
# # 2) VALGRIND
# #############################################
# echo -e "\n${YELLOW}▶ 2. Ejecutando Valgrind...${NC}"

# # Asegurar que el container esté corriendo
# if ! docker ps --format '{{.Names}}' | grep -q "^SO-TPE2$"; then
#     echo -e "${YELLOW}⚠ El contenedor SO-TPE2 no está corriendo. Intentando arrancarlo...${NC}"
#     docker start SO-TPE2 >/dev/null 2>&1
# fi

# # Si aun no está corriendo => error
# if ! docker ps --format '{{.Names}}' | grep -q "^SO-TPE2$"; then
#     echo -e "${RED}❌ ERROR: No se pudo iniciar el contenedor SO-TPE2${NC}"
#     exit 1
# fi

# docker exec SO-TPE2 bash -c "
# mkdir -p /root/logs
# cd /root/Userland/SampleCodeModule/tests

# valgrind --leak-check=full \
#     --show-leak-kinds=all \
#     --track-origins=yes \
#     ./test_mm \
#     > /root/logs/valgrind_test_mm.log 2>&1

# valgrind --leak-check=full \
#     --show-leak-kinds=all \
#     --track-origins=yes \
#     ./test_sync \
#     > /root/logs/valgrind_test_sync.log 2>&1

# valgrind --leak-check=full \
#     --show-leak-kinds=all \
#     --track-origins=yes \
#     ./test_processes \
#     > /root/logs/valgrind_test_processes.log 2>&1
# "

# # Copiar logs al host
# docker cp SO-TPE2:/root/logs/. logs/valgrind >/dev/null 2>&1

# echo -e "${GREEN}✔ Valgrind ejecutado${NC}"

# #############################################
# # 3) PVS-STUDIO
# #############################################
# echo -e "\n${YELLOW}▶ 3. Ejecutando PVS-Studio (modo strace)...${NC}"

# # eliminar rastros previos
# rm -f strace_out logs/pvs.log logs/pvs/pvs_report.html

# echo -e "${BLUE}   → Generando strace_out...${NC}"

# # IMPORTANTE: compilar de nuevo con wrapper para capturar comandos
# pvs-studio-analyzer trace -- \
#     make -j$(nproc) > logs/pvs_make.log 2>&1

# # crear carpeta pvs si no existe
# mkdir -p logs/pvs

# echo -e "${BLUE}   → Ejecutando análisis PVS...${NC}"

# # correr analizador en base al strace generado
# pvs-studio-analyzer analyze \
#     -f strace_out \
#     -o logs/pvs.log

# echo -e "${BLUE}   → Generando reporte HTML...${NC}"

# plog-converter \
#     -t fullhtml \
#     logs/pvs.log \
#     -o logs/pvs/pvs_report.html

# echo -e "${GREEN}✔ PVS-Studio completado con strace_out${NC}"


# #############################################
# # 4) RESUMEN
# #############################################
# echo -e "\n${BLUE}=== ANÁLISIS COMPLETO ===${NC}"

# echo -e "📄 build:"
# echo -e "   → logs/build_stdout.log"
# echo -e "   → logs/build_stderr.log"

# echo -e "🐘 Valgrind:"
# ls logs/valgrind/*.log 2>/dev/null || echo "   (no hay logs)"

# echo -e "🧠 PVS-Studio:"
# echo -e "   → logs/pvs.log"
# echo -e "   → logs/pvs/pvs_report.html"

# echo -e "\n${GREEN}🎉 Análisis completado${NC}"