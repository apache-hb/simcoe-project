DNS_CONTAINER_NAME="rac-dnsserver"
PUBLIC_NETWORK_NAME="rac_pub1_nw"
PRIVATE_NETWORK_NAME="rac_priv1_nw"
DNS_PUBLIC_IP="172.16.1.25"
DNS_PRIVATE_IP="192.168.17.25"

docker compose up -d ${DNS_CONTAINER_NAME} && docker compose stop ${DNS_CONTAINER_NAME}
docker network disconnect ${PUBLIC_NETWORK_NAME} ${DNS_CONTAINER_NAME}
docker network disconnect ${PRIVATE_NETWORK_NAME} ${DNS_CONTAINER_NAME}
docker network connect ${PUBLIC_NETWORK_NAME} --ip ${DNS_PUBLIC_IP} ${DNS_CONTAINER_NAME}
docker network connect ${PRIVATE_NETWORK_NAME} --ip ${DNS_PRIVATE_IP} ${DNS_CONTAINER_NAME}
docker compose start ${DNS_CONTAINER_NAME}
docker compose logs ${DNS_CONTAINER_NAME}
