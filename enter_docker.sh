#!/bin/bash

CONTAINER_NAME="SO-TPE2"

if ! docker ps -a --format "{{.Names}}" | grep -q "^${CONTAINER_NAME}$"; then
    echo "❌ El contenedor ${CONTAINER_NAME} no existe."
    exit 1
fi

docker start ${CONTAINER_NAME} >/dev/null
docker exec -it ${CONTAINER_NAME} bash
