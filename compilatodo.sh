#!/bin/bash
# Uso: ./compilatodo.sh         # Compila con MM Simple
#      ./compilatodo.sh buddy   # Compila con MM Buddy

# Configuracion del contenedor
CONTAINER_NAME="SO-TPE2"
DOCKER_IMAGE="agodio/itba-so-multi-platform:3.0"

TARGET=${1:-""}  # Parametro opcional: vacio (simple) o "buddy"

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
docker exec -it ${CONTAINER_NAME} make clean -C /root/Toolchain
docker exec -it ${CONTAINER_NAME} make clean -C /root
docker exec -it ${CONTAINER_NAME} make -C /root/Toolchain
docker exec -it ${CONTAINER_NAME} make $TARGET -C /root
docker exec -it ${CONTAINER_NAME} chmod 777 /root/Image/x64BareBonesImage.qcow2
docker stop ${CONTAINER_NAME}