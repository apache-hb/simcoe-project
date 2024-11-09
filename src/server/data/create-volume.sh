STORAGE_PRIVATE_IP="192.168.17.80"
STORAGE_VOLUME_NAME="racstorage"
ORACLE_DBNAME="ORCLCDB"

mkdir -p container/asm/$ORACLE_DBNAME
rm -rf container/asm/$ORACLE_DBNAME/asm_disk0*

docker volume create --driver local \
  --opt type=nfs \
  --opt o=addr=${STORAGE_PRIVATE_IP},rw,bg,hard,tcp,vers=3,timeo=600,rsize=32768,wsize=32768,actimeo=0 \
  --opt device=${STORAGE_PRIVATE_IP}:/oradata \
  ${STORAGE_VOLUME_NAME}
