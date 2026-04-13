# Guía de Pruebas — ExtreamFS
**Mariano Roberto Rac Noguera | 2022101149**

Orden de pruebas, qué esperar ver en el frontend y cómo verificar que cada comando funciona correctamente.

---

## Antes de empezar

1. Verificar que el backend está corriendo:
   ```
   http://23.23.109.9:8080/status  →  debe responder {"status":"ok"}
   ```
2. Abrir el frontend:
   ```
   http://extreamfs-frontend-849279003367.s3-website-us-east-1.amazonaws.com
   ```
3. El script completo de pruebas está en `Archivo_De_Prueba_P2.smia`.  
   Puedes cargarlo con el botón de archivo en el terminal y ejecutarlo de una sola vez con **"Ejecutar Script"**, o ir sección por sección pegando los comandos en el área de entrada y presionando **"Ejecutar"**.

---

## Fase 1 — Discos y particiones

### Qué se prueba
- `mkdisk`: crear discos virtuales `.mia`
- `rmdisk`: borrar discos
- `fdisk`: crear particiones primarias/extendidas/lógicas
- `mount`: montar particiones
- `mounted`: listar particiones montadas

### Cómo ejecutarlo
Pegar o cargar la sección `SPRINT 1 - REVISION P1` del script en el terminal del frontend y ejecutar.

### Qué ver en el terminal (Salida)
```
Disco creado: /home/ubuntu/Calificacion_MIA/Discos/Disco1.mia
ERROR: parámetro inválido ...        ← líneas de error esperadas
Partición creada: Part11
Partición montada: 491A
...
```
- Las líneas de error (`ERROR:`) en `mkdisk -param=x` y `fdisk` sobre disco inexistente son **correctas y esperadas** — prueban el manejo de errores.
- El comando `mounted` al final debe listar: `491A, 492A, 491B, 492B, 211A, 211B`.

### Qué ver en el Visualizador
1. Presionar **Refrescar** en el Visualizador del Sistema de Archivos.
2. Deben aparecer los íconos de disco: `Disco1.mia`, `Disco3.mia`, `Disco4.mia`.
3. Hacer clic en `Disco1.mia` → aparecen las particiones `Part11 (491A)` y `Part12 (492A)`.
4. En este punto las particiones no tienen sistema de archivos aún — si entras al explorador no habrá nada.

---

## Fase 2 — Formateo EXT2 y EXT3 (MKFS)

### Qué se prueba
- `mkfs -fs=2fs`: formatear partición como EXT2
- `mkfs -fs=3fs`: formatear partición como EXT3
- Error con ID no montado

### Cómo ejecutarlo
Sección `SPRINT 2 - MKFS` del script.

### Qué ver en el terminal
```
Partición 491A formateada como EXT2
Partición 492A formateada como EXT2
ERROR: ID 499Z no montado
Partición 491B formateada como EXT3
Partición 492B formateada como EXT3
```

### Qué ver en el Visualizador
Después del mkfs, las particiones ya tienen estructura. Si entras a `491A`:
- Verás la carpeta raíz `/` (carpeta vacía por ahora, sin usuarios ni archivos).
- Esto confirma que el sistema de archivos se inicializó correctamente.

---

## Fase 3 — Login, usuarios y grupos

### Qué se prueba
- `login` / `logout`
- `mkgrp` / `rmgrp`: crear y eliminar grupos
- `mkusr` / `rmusr`: crear y eliminar usuarios
- `chgrp`: cambiar grupo de un usuario
- `cat /users.txt`: ver el archivo de usuarios
- Errores: sesión ya activa, grupo duplicado, usuario duplicado, grupo inexistente

### Cómo ejecutarlo
Sección `SISTEMA DE ARCHIVOS: USUARIOS Y DIRS` del script.

### Qué ver en el terminal
- Después de cada `cat -file1=/users.txt` debe mostrarse el contenido del archivo de usuarios, parecido a:
  ```
  1,root,root,123
  2,usuarios,,
  3,adm,,
  1,usuario1,root,password
  2,user1,usuarios,abc
  ...
  ```
- `ERROR: sesión ya activa` al intentar doble login — esperado.
- `ERROR: grupo ya existe` en el segundo `mkgrp -name=sys` — esperado.

### Qué ver en la pantalla principal
- El botón en la esquina superior derecha cambia de **"Iniciar Sesión"** (azul) a **"Cerrar Sesión"** (rojo) cuando el login es exitoso.
- La barra de sesión debajo del header muestra: `root | Grupo: root | Partición: 491A`.

### Alternativa con el formulario de Login
En vez de usar `login` por terminal, puedes usar la pantalla de Login:
1. Presionar **"Iniciar Sesión"** (esquina superior derecha).
2. Ingresar: ID Partición = `491A`, Usuario = `root`, Contraseña = `123`.
3. Presionar Submit → regresa al home con la sesión activa.

---

## Fase 4 — Directorios y archivos

### Qué se prueba
- `mkdir` / `mkdir -p`: crear directorios (simple y recursivo)
- `mkfile`: crear archivos con tamaño aleatorio o contenido desde archivo
- `cat`: leer contenido de un archivo
- Errores: ruta inexistente, tamaño negativo

### Cómo ejecutarlo
Continuación de la sección de usuarios (después del login de root en 491A).

### Qué ver en el terminal
- `mkdir -path=/bin` → `Directorio creado: /bin`
- `mkdir -p -path=/home/archivos/carpeta1/carpeta2/carpeta3/carpeta4/carpeta5` → crea toda la jerarquía de una vez
- `mkfile -size=75` → crea archivo con 75 bytes de contenido aleatorio
- `cat -file1=/home/archivos/user/docs/Tarea3.txt` → debe mostrar el contenido del archivo `NAME.txt` que subiste al EC2 (`Contenido de prueba para Tarea3 MIA Proyecto 2`)

### Qué ver en el Visualizador
Esta es la parte visual más importante:

1. Presionar **Refrescar** en el Visualizador.
2. Disco `Disco1.mia` → Partición `Part11 (491A)`.
3. En `/` verás íconos de carpeta: `bin`, `home`.
4. Entrar a `home` → `archivos` → `user` → `docs`.
5. Verás los íconos de archivo: `Tarea2.txt`, `Tarea3.txt`, `Tarea.txt` (si no fue renombrado aún).
6. Hacer clic en `Tarea3.txt` → se abre el **Visualizador de Archivos** mostrando el contenido del archivo.
7. La barra de ruta arriba muestra el path completo: `/home/archivos/user/docs/Tarea3.txt`.

---

## Fase 5 — Comandos nuevos P2 (sobre EXT2)

### Qué se prueba
- `rename`: renombrar archivos/carpetas
- `copy`: copiar archivos/carpetas
- `move`: mover archivos/carpetas
- `remove`: eliminar archivos/carpetas
- `find`: buscar por nombre con wildcards
- `chmod`: cambiar permisos (ugo=755, recursivo)
- `chown`: cambiar propietario (recursivo)

### Cómo ejecutarlo
Sección `SPRINT 2 - COMANDOS NUEVOS P2`.

### Qué ver en el terminal
```
Archivo renombrado a TareaRenombrada.txt
Archivo copiado a /home/archivos/carpeta1
ERROR: directorio destino no existe
...
find -path=/home -name=*.txt → lista todos los .txt en /home
```

### Qué verificar en el Visualizador después de cada bloque
- **Después de rename:** navegar a `/home/archivos/user/docs` — `Tarea.txt` ya no existe, aparece `TareaRenombrada.txt`.
- **Después de copy:** navegar a `/home/archivos/carpeta1` — debe aparecer `Tarea2.txt` copiado ahí.
- **Después de move:** `TareaRenombrada.txt` desaparece de `docs` y aparece en `carpeta1`.
- **Después de remove:** `Tarea2.txt` en `docs` ya no existe; `carpeta3` y su contenido tampoco.
- **Permisos (chmod/chown):** en los íconos del explorador verás el string de permisos debajo del nombre, como `rwxr-xr-x`.

---

## Fase 6 — EXT3 y Journaling (sobre 491B)

### Qué se prueba
- Operaciones sobre partición EXT3 (`491B`)
- `journaling -id=491B`: ver el diario de operaciones
- `loss -id=491B`: simular pérdida del sistema y recuperación

### Cómo ejecutarlo
Sección `SPRINT 2 - EXT3 y JOURNALING`.

### Qué ver en el terminal
Después de `journaling -id=491B` debe aparecer algo como:
```
Journal 491B:
[1] mkdir /home/user1/documentos — 2026-01-15
[2] mkdir /home/user1/proyectos/mia — 2026-01-15
[3] mkfile /home/user1/documentos/nota.txt — 2026-01-15
[4] rename /home/user1/documentos/nota.txt → nota_v2.txt — 2026-01-15
...
```
Después de `loss` y un segundo `journaling -id=491B`, el journal se reinicia (vacío o con entradas nuevas) — confirma que el sistema de pérdida funciona.

### Qué ver en el Visualizador — botón Journal
1. En el Visualizador, entrar a Disco → `Disco3.mia` → `Part31 (491B)`.
2. Navegar a `/` o cualquier carpeta.
3. Presionar el botón **Journal** en la barra de ruta.
4. Se despliega el panel del diario mostrando todas las operaciones registradas.

---

## Fase 7 — FDISK ADD y DELETE

### Qué se prueba
- `unmount`: desmontar partición
- `fdisk -delete=fast`: eliminar partición rápido (solo tabla)
- `fdisk -delete=full`: eliminar partición y limpiar con `\0`
- `fdisk -add=N`: agregar/quitar espacio a partición existente
- Errores: partición montada al intentar borrar, espacio insuficiente

### Cómo ejecutarlo
Sección `SPRINT 2 - FDISK ADD/DELETE`.

### Qué ver en el terminal
```
Partición 492A desmontada
ERROR: ID 499Z no está montado
Partición Part13 eliminada (fast)
Partición Part14 eliminada (full)
ERROR: no se puede eliminar Part11 — está montada
5 MB agregados a Part11
ERROR: no hay espacio suficiente en el disco
```

### Qué ver en el Visualizador
- Presionar **Refrescar**.
- `Disco1.mia` ahora solo debe mostrar `Part11 (491A)` — las otras particiones fueron eliminadas o desmontadas.

---

## Fase 8 — Reportes

Los reportes generan imágenes/texto en el EC2 en `/home/ubuntu/Calificacion_MIA/Reportes/`. Para verlos tienes que acceder por SSH o haberlos descargado antes. El frontend no los muestra directamente; el backend los genera como archivos.

Tipos de reporte que se generan:
- `disk` — distribución del disco (imagen)
- `mbr` — tabla de particiones MBR (imagen)
- `inode` — tabla de inodos (imagen)
- `block` — bloques de datos (imagen)
- `sb` — superbloque (imagen)
- `bm_inode` — bitmap de inodos (texto)
- `bm_block` — bitmap de bloques (texto)
- `tree` — árbol del sistema de archivos (imagen)
- `ls` — listado con permisos (imagen)
- `file` — bloques de un archivo específico (texto)

Para descargar los reportes del EC2:
```bash
scp -i ~/.ssh/extreamfs-key.pem \
  ubuntu@23.23.109.9:~/Calificacion_MIA/Reportes/* \
  ~/Desktop/reportes_mia/
```

---

## Flujo completo de demostración (para la entrega)

Este es el orden recomendado para mostrar el proyecto funcionando de punta a punta:

```
1. Abrir frontend → mostrar que el backend responde (barra de sesión en rojo)
2. Pegar y ejecutar sección MKDISK + FDISK + MOUNT en el terminal
3. Refrescar el Visualizador → mostrar íconos de disco
4. Seleccionar disco → mostrar íconos de partición
5. Ejecutar MKFS en terminal
6. Login desde la pantalla de Login (no por comando)
   → mostrar barra de sesión en verde y botón "Cerrar Sesión"
7. Ejecutar MKDIR + MKFILE en terminal
8. Refrescar Visualizador → navegar carpetas → hacer clic en archivo → ver contenido
9. Ejecutar RENAME + COPY + MOVE + REMOVE → refrescar y verificar cambios
10. Cambiar a partición 491B (EXT3)
11. Ejecutar MKDIR + MKFILE + journaling → ver Journal en el visualizador
12. Ejecutar LOSS → ejecutar journaling de nuevo → confirmar recuperación
13. Logout → botón vuelve a "Iniciar Sesión"
```

---

## Errores esperados (no son bugs)

| Comando | Error | Por qué es correcto |
|---|---|---|
| `mkdisk -param=x ...` | `ERROR: parámetro inválido` | Valida parámetros desconocidos |
| `fdisk` en disco inexistente | `ERROR: disco no encontrado` | El disco no existe |
| `fdisk` 5ta partición primaria | `ERROR: límite de particiones` | MBR solo admite 4 primarias |
| `mount` partición ya montada | `ERROR: ya está montada` | No se puede montar dos veces |
| `login` con sesión activa | `ERROR: sesión ya activa` | Solo una sesión a la vez |
| `mkgrp` duplicado | `ERROR: grupo ya existe` | No se permiten duplicados |
| `mkusr` en grupo inexistente | `ERROR: grupo no existe` | Integridad referencial |
| `mkfile -size=-45` | `ERROR: tamaño inválido` | El tamaño debe ser positivo |
| `fdisk -delete` en montada | `ERROR: partición montada` | Hay que desmontar primero |
| `fdisk -add=100M` sin espacio | `ERROR: espacio insuficiente` | No cabe en el disco |
| `unmount -id=499Z` | `ERROR: ID no montado` | El ID no existe |
