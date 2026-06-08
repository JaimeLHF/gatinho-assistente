#!/bin/bash
# PostgreSQL backup script — run via cron or manually
# Usage: ./infra/backup-db.sh
# Backups are stored in ./backups/ with 7-day retention

set -euo pipefail

BACKUP_DIR="$(dirname "$0")/../backups"
COMPOSE_FILE="$(dirname "$0")/../docker-compose.prod.yml"
DATE=$(date +%Y%m%d_%H%M%S)
FILENAME="gatinho_pet_${DATE}.sql.gz"
RETENTION_DAYS=7

mkdir -p "${BACKUP_DIR}"

echo "[$(date)] Starting backup..."

docker compose -f "${COMPOSE_FILE}" exec -T postgres \
  pg_dump -U root gatinho_pet \
  | gzip > "${BACKUP_DIR}/${FILENAME}"

echo "[$(date)] Backup saved: ${FILENAME} ($(du -h "${BACKUP_DIR}/${FILENAME}" | cut -f1))"

# Clean old backups
find "${BACKUP_DIR}" -name "gatinho_pet_*.sql.gz" -mtime +${RETENTION_DAYS} -delete
echo "[$(date)] Cleaned backups older than ${RETENTION_DAYS} days"
