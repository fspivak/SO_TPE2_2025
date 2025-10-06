#!/bin/bash
# Uso: ./compilatodo.sh         # Compila con MM Simple
#      ./compilatodo.sh buddy   # Compila con MM Buddy

TARGET=${1:-""}  # Parametro opcional: vacio (simple) o "buddy"

docker start SO-TPE2
docker exec -it SO-TPE2 make clean -C /root/Toolchain
docker exec -it SO-TPE2 make clean -C /root
docker exec -it SO-TPE2 make -C /root/Toolchain
docker exec -it SO-TPE2 make $TARGET -C /root
docker exec -it SO-TPE2 chmod 777 /root/Image/x64BareBonesImage.qcow2
docker stop SO-TPE2