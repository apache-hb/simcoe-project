STORAGE_CONTAINER_NAME=racnode-storage

docker compose up -d ${STORAGE_CONTAINER_NAME}
docker compose logs -f ${STORAGE_CONTAINER_NAME}
