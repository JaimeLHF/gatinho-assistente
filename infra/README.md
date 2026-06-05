# Deploy para Produção

## Pré-requisitos

- Servidor com Docker e Docker Compose instalados
- Domínio apontando para o IP do servidor (DNS A record)
- Portas 80 e 443 abertas no firewall

## Passo a passo

### 1. Clonar e configurar

```bash
git clone <repo-url> && cd gatinho-assistente

# Copiar templates de env
cp .env.production.example .env.production
cp api/.env.production.example api/.env.production
cp web/.env.production.example web/.env.production
```

### 2. Editar secrets

```bash
# Gerar secrets seguros
openssl rand -base64 48  # para JWT_ACCESS_SECRET
openssl rand -base64 48  # para JWT_REFRESH_SECRET
openssl rand -base64 32  # para MYSQL_ROOT_PASSWORD
```

Editar `.env.production` e `api/.env.production` com os valores gerados.
Substituir `yourdomain.com` pelo domínio real em `api/.env.production` (CORS_ORIGIN).

### 3. Configurar domínio no nginx

Substituir `${DOMAIN}` pelo domínio real nos arquivos:
- `infra/nginx/nginx.conf`
- `infra/nginx/nginx-initial.conf`

Ou usar o script automático (passo 4).

### 4. Obter certificado SSL

```bash
./infra/init-ssl.sh seudominio.com seuemail@example.com
```

### 5. Subir tudo

```bash
docker compose -f docker-compose.prod.yml --env-file .env.production up -d
```

### 6. Rodar migrations

```bash
docker compose -f docker-compose.prod.yml exec api \
  npx prisma migrate deploy
```

## Backup

```bash
# Manual
MYSQL_ROOT_PASSWORD=<senha> ./infra/backup-mysql.sh

# Cron (diário às 3h)
0 3 * * * cd /path/to/gatinho-assistente && MYSQL_ROOT_PASSWORD=<senha> ./infra/backup-mysql.sh >> /var/log/gatinho-backup.log 2>&1
```

## Atualizando

```bash
git pull
docker compose -f docker-compose.prod.yml --env-file .env.production up -d --build
docker compose -f docker-compose.prod.yml exec api npx prisma migrate deploy
```
