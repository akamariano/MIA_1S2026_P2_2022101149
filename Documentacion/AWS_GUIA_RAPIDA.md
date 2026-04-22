# AWS — Guía Rápida
**Mariano Roberto Rac Noguera | 2022101149**

---

## Mis URLs

| Qué | URL |
|---|---|
| Frontend (S3) | http://extreamfs-frontend-849279003367.s3-website-us-east-1.amazonaws.com |
| Backend API (EC2) | http://13.218.255.179:8080 |
| Backend status | http://13.218.255.179:8080/status |

> **`13.218.255.179` es una Elastic IP — no cambia aunque reinicies la instancia.**

---

## Mis recursos

| Recurso | Valor |
|---|---|
| EC2 Instance ID | `i-06056fe478a750f4f` |
| EC2 tipo | `t3.micro` |
| EC2 región | `us-east-1` (N. Virginia) |
| **Elastic IP** | `13.218.255.179` (estática) |
| Elastic IP Allocation ID | `eipalloc-0884d6576cb90f39c` |
| Elastic IP Association ID | (se asigna al encender el EC2) |
| S3 Bucket | `extreamfs-frontend-849279003367` |
| SSH Key | `~/.ssh/extreamfs-key.pem` |

---

## Plan para la calificación

**La Elastic IP `13.218.255.179` ya está asociada — IP fija, no cambia aunque reinicie el EC2.**

Dejar el EC2 encendido hasta después de la calificación. Un t3.micro encendido hasta el 23 de abril = ~**$0.50 total**.

### Después de la calificación — apagar todo ($0)

```bash
# Obtener association ID de la Elastic IP activa
ASSOC_ID=$(aws ec2 describe-addresses --region us-east-1 \
  --query 'Addresses[?InstanceId==`i-06056fe478a750f4f`].AssociationId' \
  --output text)

ALLOC_ID=$(aws ec2 describe-addresses --region us-east-1 \
  --query 'Addresses[?InstanceId==`i-06056fe478a750f4f`].AllocationId' \
  --output text)

# Desasociar y liberar Elastic IP
aws ec2 disassociate-address --association-id $ASSOC_ID --region us-east-1
aws ec2 release-address --allocation-id $ALLOC_ID --region us-east-1

# Detener el EC2
aws ec2 stop-instances --instance-ids i-06056fe478a750f4f --region us-east-1

echo "Todo apagado — costo $0"
```

---

## Costos — regla de oro

| Situación | Costo |
|---|---|
| EC2 encendido (Free Tier activo) | **$0.00** |
| EC2 encendido sin Free Tier | ~**$0.008/hora** (~$0.19/día) |
| Elastic IP asociada a EC2 encendido | **$0.00** |
| Elastic IP sin instancia asociada | **~$0.005/hora** — LIBERAR SIEMPRE |
| S3 siempre | **$0.00** |

> **Regla:** Si apagas el EC2, libera la Elastic IP antes. Si lo dejas encendido, no toques nada.

---

## Ver la carpeta CalificacionMIA (SSH)

La consola de AWS **no permite navegar el filesystem** del EC2 — solo muestra métricas y logs. Para ver los archivos necesitas SSH:

```bash
# Conectarse al EC2
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@13.218.255.179

# Ver la carpeta de calificación (actualiza la IP si cambió)
ls -lh /home/ubuntu/Calificacion_MIA/
```

Deberías ver los discos virtuales creados por el script de pruebas:
```
Disco1.mia   Disco2.mia   Disco3.mia   Disco4.mia   Disco5.mia
```

Si la carpeta no existe (antes de correr las pruebas por primera vez):
```bash
mkdir -p /home/ubuntu/Calificacion_MIA
```

El comando `mkdisk` del script crea los archivos `.mia` automáticamente en esa ruta.

---

## Verificar que todo anda

```bash
# Estado del EC2 e IP actual
aws ec2 describe-instances \
  --instance-ids i-06056fe478a750f4f \
  --region us-east-1 \
  --query 'Reservations[0].Instances[0].{Estado:State.Name,IP:PublicIpAddress}' \
  --output table

# Verificar que el backend responde
curl -s http://13.218.255.179:8080/status   # actualiza IP si cambió

# SSH al EC2 (para ver logs o debuggear)
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@13.218.255.179

# Ver logs del servicio backend en el EC2
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@13.218.255.179 "sudo journalctl -u extreamfs -n 50"
```

---

## Redeploy rápido (cuando cambio código)

```bash
# Backend: compilar localmente y copiar el binario al EC2
cd /home/mariano/MIA_1S2026_P2_2022101149/frontend/backend/build
make -j$(nproc)
scp -i ~/.ssh/extreamfs-key.pem extreamfs ubuntu@13.218.255.179:~/extreamfs/extreamfs
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@13.218.255.179 "sudo systemctl restart extreamfs"

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
AWS S3 ─────────────────────────────────────────────
    extreamfs-frontend-849279003367
    Frontend Next.js (estático)
─────────────────────────────────────────────────────
                                     API calls
                                          ▼
AWS EC2 t3.micro ────────────────────────────────────
    Ubuntu 22.04 | IP dinámica (ver sección URLs) | Puerto: 8080
    Backend C++ (extreamfs) — servicio systemd
─────────────────────────────────────────────────────
                    │
                    │ archivos .mia
                    ▼
    /home/ubuntu/Calificacion_MIA/
    Discos virtuales EXT2 / EXT3 (Disco1.mia … Disco5.mia)
```
