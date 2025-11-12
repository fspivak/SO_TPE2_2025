#!/bin/bash
# Uso: ./compilatodo.sh         # Compila con MM Simple
#      ./compilatodo.sh buddy   # Compila con MM Buddy

# Configuracion del contenedor
CONTAINER_NAME="SO-TPE2"
DOCKER_IMAGE="agodio/itba-so-multi-platform:3.0"

TARGET=${1:-""}  # Parametro opcional: vacio (simple) o "buddy"

# Detectar si estamos en un contexto interactivo (TTY disponible)
if [ -t 0 ]; then
    DOCKER_EXEC="docker exec -it"
else
    # Modo no interactivo (ej: pre-commit hook, GitHub Desktop)
    DOCKER_EXEC="docker exec"
fi

# Verificar si la imagen Docker existe, si no descargarla
if ! docker images --format "table {{.Repository}}:{{.Tag}}" | grep -q "^${DOCKER_IMAGE}$"; then
    echo "Imagen ${DOCKER_IMAGE} no existe, descargando..."
    docker pull ${DOCKER_IMAGE}
fi

# Verificar si el contenedor existe, si no crearlo
if ! docker ps -a --format "table {{.Names}}" | grep -q "^${CONTAINER_NAME}$"; then
    echo "Contenedor ${CONTAINER_NAME} no existe, creando..."
    docker run -d --name ${CONTAINER_NAME} -v $(pwd):/root ${DOCKER_IMAGE} tail -f /dev/null
else
    docker start ${CONTAINER_NAME}
fi
${DOCKER_EXEC} ${CONTAINER_NAME} make clean -C /root/Toolchain
${DOCKER_EXEC} ${CONTAINER_NAME} make clean -C /root
${DOCKER_EXEC} ${CONTAINER_NAME} make -C /root/Toolchain
${DOCKER_EXEC} ${CONTAINER_NAME} make $TARGET -C /root
${DOCKER_EXEC} ${CONTAINER_NAME} chmod 777 /root/Image/x64BareBonesImage.qcow2
docker stop ${CONTAINER_NAME}