# AWS — Guía Rápida
**Mariano Roberto Rac Noguera | 2022101149**

---

## Mis URLs

| Qué | URL |
|---|---|
| Frontend (S3) | http://extreamfs-frontend-849279003367.s3-website-us-east-1.amazonaws.com |
| Backend API (EC2) | http://23.23.109.9:8080 |
| Backend status | http://23.23.109.9:8080/status |

---

## Mis recursos

| Recurso | Valor |
|---|---|
| EC2 Instance ID | `i-06056fe478a750f4f` |
| EC2 tipo | `t3.micro` |
| EC2 región | `us-east-1` (N. Virginia) |
| Elastic IP | `23.23.109.9` |
| Elastic IP Allocation ID | `eipalloc-0d41b20f8319ab766` |
| Elastic IP Association ID | `eipassoc-073c5cdb430d952e9` |
| S3 Bucket | `extreamfs-frontend-849279003367` |
| SSH Key | `~/.ssh/extreamfs-key.pem` |

---

## Costos — regla de oro

| Situación | Costo |
|---|---|
| EC2 encendido + Elastic IP asignada | **$0.00** (Free Tier) |
| EC2 apagado pero Elastic IP sin liberar | **~$0.005/hora** — EVITAR |
| EC2 apagado + Elastic IP liberada | **$0.00** |
| S3 siempre | **$0.00** |

> **Si apago el EC2, libero la Elastic IP. Si lo dejo encendido, no toco nada.**

---

## Apagar todo ($0)

```bash
# 1. Detener el EC2
aws ec2 stop-instances --instance-ids i-06056fe478a750f4f --region us-east-1

# 2. Desasociar la Elastic IP
aws ec2 disassociate-address \
  --association-id eipassoc-073c5cdb430d952e9 \
  --region us-east-1

# 3. Liberar la Elastic IP
aws ec2 release-address \
  --allocation-id eipalloc-0d41b20f8319ab766 \
  --region us-east-1

echo "Todo apagado — costo $0"
```

---

## Encender de nuevo

```bash
# 1. Encender el EC2
aws ec2 start-instances --instance-ids i-06056fe478a750f4f --region us-east-1

# 2. Esperar que esté running
aws ec2 wait instance-running --instance-ids i-06056fe478a750f4f --region us-east-1

# 3. Asignar nueva Elastic IP
ALLOCATION=$(aws ec2 allocate-address --domain vpc --region us-east-1 \
  --query '{AllocationId:AllocationId,PublicIp:PublicIp}' --output json)
echo $ALLOCATION

ALLOC_ID=$(echo $ALLOCATION | python3 -c "import sys,json; print(json.load(sys.stdin)['AllocationId'])")

# 4. Asociar la IP al EC2
aws ec2 associate-address \
  --instance-id i-06056fe478a750f4f \
  --allocation-id $ALLOC_ID \
  --region us-east-1

# 5. Ver la nueva IP
NEW_IP=$(aws ec2 describe-addresses \
  --allocation-ids $ALLOC_ID \
  --query 'Addresses[0].PublicIp' \
  --output text --region us-east-1)
echo "Nueva IP: $NEW_IP"

# 6. Actualizar el .env.production con la nueva IP
sed -i "s|NEXT_PUBLIC_API_URL=.*|NEXT_PUBLIC_API_URL=http://$NEW_IP:8080|" \
  /home/mariano/MIA_1S2026_P2_2022101149/frontend/.env.production

# 7. Rebuild y redeploy del frontend
cd /home/mariano/MIA_1S2026_P2_2022101149/frontend
npm run build
aws s3 sync out/ s3://extreamfs-frontend-849279003367/ --delete --region us-east-1 --quiet

echo "Listo. Frontend: http://extreamfs-frontend-849279003367.s3-website-us-east-1.amazonaws.com"
```

> **Nota:** Cada vez que apago y prendo el EC2 la Elastic IP cambia. Por eso hay que hacer el rebuild del frontend con la nueva IP. El backend en el EC2 inicia solo (systemd).

---

## Verificar que todo anda

```bash
# Estado del EC2
aws ec2 describe-instances \
  --instance-ids i-06056fe478a750f4f \
  --region us-east-1 \
  --query 'Reservations[0].Instances[0].{Estado:State.Name,IP:PublicIpAddress}' \
  --output table

# Verificar que el backend responde
curl -s http://23.23.109.9:8080/status

# SSH al EC2 (para ver logs o debuggear)
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@23.23.109.9

# Ver logs del servicio backend en el EC2
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@23.23.109.9 "sudo journalctl -u extreamfs -n 50"
```

---

## Redeploy rápido (cuando cambio código)

```bash
# Backend: compilar localmente y copiar el binario al EC2
cd /home/mariano/MIA_1S2026_P2_2022101149/frontend/backend/build
make -j$(nproc)
scp -i ~/.ssh/extreamfs-key.pem extreamfs ubuntu@23.23.109.9:~/extreamfs/extreamfs
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@23.23.109.9 "sudo systemctl restart extreamfs"

# Frontend: rebuild y sync a S3
cd /home/mariano/MIA_1S2026_P2_2022101149/frontend
npm run build
aws s3 sync out/ s3://extreamfs-frontend-849279003367/ --delete --region us-east-1 --quiet
```

---

## Arquitectura

```
Navegador
    │
    ▼
AWS S3  ──────────────────────────────────────────
    extreamfs-frontend-849279003367               │
    Frontend Next.js (estático)                   │
─────────────────────────────────────────────────  │
                                     API calls     │
                                          ▼
AWS EC2 t3.micro ─────────────────────────────────
    Ubuntu 22.04 | IP: 23.23.109.9 | Puerto: 8080
    Backend C++ (extreamfs) — servicio systemd
──────────────────────────────────────────────────
                    │
                    │ archivos .mia
                    ▼
    /home/ubuntu/extreamfs/
    Discos virtuales EXT2 / EXT3
```
