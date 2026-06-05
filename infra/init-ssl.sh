#!/bin/bash
# First-time SSL certificate setup for Let's Encrypt
# Usage: ./infra/init-ssl.sh yourdomain.com your@email.com

set -euo pipefail

DOMAIN="${1:?Usage: $0 <domain> <email>}"
EMAIL="${2:?Usage: $0 <domain> <email>}"
COMPOSE_FILE="docker-compose.prod.yml"

echo "==> Setting up SSL for ${DOMAIN}"

# Step 1: Replace domain placeholder in nginx configs
sed -i "s/\${DOMAIN}/${DOMAIN}/g" infra/nginx/nginx.conf
sed -i "s/\${DOMAIN}/${DOMAIN}/g" infra/nginx/nginx-initial.conf

# Step 2: Start with HTTP-only nginx to respond to ACME challenge
echo "==> Starting nginx with initial (HTTP-only) config..."
cp infra/nginx/nginx.conf infra/nginx/nginx-ssl.conf.bak
cp infra/nginx/nginx-initial.conf infra/nginx/nginx.conf

docker compose -f ${COMPOSE_FILE} up -d nginx

# Step 3: Obtain certificate
echo "==> Requesting certificate from Let's Encrypt..."
docker compose -f ${COMPOSE_FILE} run --rm certbot certonly \
  --webroot \
  --webroot-path=/var/www/certbot \
  --email "${EMAIL}" \
  --agree-tos \
  --no-eff-email \
  -d "${DOMAIN}"

# Step 4: Restore full SSL config
echo "==> Switching to full SSL config..."
cp infra/nginx/nginx-ssl.conf.bak infra/nginx/nginx.conf
rm infra/nginx/nginx-ssl.conf.bak

# Step 5: Restart everything
echo "==> Restarting all services..."
docker compose -f ${COMPOSE_FILE} down
docker compose -f ${COMPOSE_FILE} up -d

echo "==> Done! https://${DOMAIN} should be live."
