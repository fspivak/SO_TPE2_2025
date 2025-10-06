#!/bin/bash
docker start SO-TPE2
docker exec -it SO-TPE2 make clean -C /root/Toolchain
docker exec -it SO-TPE2 make clean -C /root
docker exec -it SO-TPE2 make -C /root/Toolchain
docker exec -it SO-TPE2 make -C /root
docker exec -it SO-TPE2 chmod 777 /root/Image/x64BareBonesImage.qcow2
docker stop SO-TPE2