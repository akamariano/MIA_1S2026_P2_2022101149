# Guía de Pruebas — ExtreamFS
**Mariano Roberto Rac Noguera | 2022101149**

Notas personales para verificar el proyecto de punta a punta: qué ejecutar, qué debe salir en el terminal, y qué revisar visualmente en el frontend.

---

## URLs activas

| Servicio | URL |
|---|---|
| **Frontend** | `http://extreamfs-frontend-849279003367.s3-website-us-east-1.amazonaws.com` |
| **Backend health** | `http://13.218.255.179:8080/status` → debe responder `{"status":"ok"}` |
| **Script de pruebas** | `Archivo_De_Prueba_P2.smia` (raíz del proyecto) |

> **`13.218.255.179` es una Elastic IP — no cambia aunque se reinicie el EC2.**

---

## ⚡ Guía rápida — Checks visuales del frontend

Ejecutar el script completo y luego revisar estos puntos en el visualizador. Son los que más fácil se ven a primera vista.

### 1. Sesión y barra de estado
| Qué hacer | Qué debe verse |
|---|---|
| Abrir el frontend sin login | Barra superior: `Sin sesión activa` |
| Clic en **"Iniciar Sesión"** → ID=`491A`, user=`root`, pass=`123` | Barra cambia a `root \| Grupo: root \| Partición: 491A` |
| Botón esquina superior derecha | Cambia de **"Iniciar Sesión"** a **"Cerrar Sesión"** |
| Clic en **"Cerrar Sesión"** | Barra vuelve a `Sin sesión activa` |

### 2. Visualizador de discos y particiones
| Qué hacer | Qué debe verse |
|---|---|
| Clic en **Refrescar** (después de correr el script) | Aparecen `Disco1.mia`, `Disco2.mia`, `Disco3.mia`, `Disco4.mia`, `Disco5.mia` |
| Clic en `Disco1.mia` | Particiones: `Part11 (491A)` |
| Clic en `Disco3.mia` | Particiones: `Part31 (491B)`, `Part32 (492B)` |
| Clic en `Disco4.mia` | Partición: `Part41 (491C)` |
| Clic en `Disco2.mia` | Particiones: `Part21 (491D)`, `Part22 (492D)` |
| Clic en `Disco5.mia` | Partición: `Part51 (491E)` |

### 3. Explorador de archivos — navegación
| Qué hacer | Qué debe verse |
|---|---|
| Entrar a `491A` → `/` | Carpetas: `bin`, `home` |
| Navegar `/home/archivos/user/docs` | Archivos: `Tarea3.txt`, `entrada.txt` (en usac/...), `MiArchivo.txt`, `ConContenido.txt` |
| Clic en `Tarea3.txt` | Panel derecho muestra el contenido: `Contenido de prueba para Tarea3 MIA Proyecto 2` |
| Navegar `/home/archivos/carpeta1` | Aparece `Tarea2.txt` (copiado), `TareaRenombrada.txt` (movido), `carpeta2/` |
| Navegar `/home/archivos/carpeta1/carpeta2` | Contiene `dirOrigen/` (movido en Sprint 3) y carpeta `usac/` (copiada) |
| Entrar a `491B` → `/home/user1/proyectos` | Carpetas: `mia/`, `nota_v2.txt` (movido desde documentos) |
| Entrar a `491D` → `/home/disco2/subdir` | Archivo: `archivo.txt` |
| Entrar a `491E` → `/srv/datos` | Archivo: `info_v2.txt` (renombrado desde info.txt) |

### 4. Contenido de archivos
| Qué hacer | Qué debe verse |
|---|---|
| Clic en `Tarea3.txt` (491A) | Contenido del archivo `NAME.txt` del EC2 |
| Clic en `ConContenido.txt` (491A) | Mismo contenido que `NAME.txt` (creado con `-cont`) |
| Clic en `archivo.txt` en 491D | Contenido generado aleatoriamente (bytes de relleno) |

### 5. Permisos y propietario en el explorador
Navegar a `491A → /home/archivos/user/docs` y verificar la columna de permisos:
| Archivo | Permisos | Propietario |
|---|---|---|
| `Tarea3.txt` | `rwxr-xr-x` (755) | `user1` |
| `MiArchivo.txt` | `rw-------` (600) | `user1` |
| `ConContenido.txt` | `--x--x--x` (111) | `usuario1` |

### 6. Journal de EXT3 (partición 491B)
| Qué hacer | Qué debe verse |
|---|---|
| Entrar a `491B` → cualquier carpeta | Aparece botón **Journal** en la barra de ruta |
| Clic en **Journal** | Panel con lista de operaciones: mkdir, mkfile, rename, copy, move, remove, chmod, chown |
| Cada entrada del journal | Muestra: operación, ruta afectada, número de inodo, fecha |

### 7. Reportes — links directos al navegador

Base: `http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/`

#### Reportes finales (estado completo del sistema)
| Reporte | Link |
|---|---|
| Árbol EXT2 final (491A) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_final_r1_tree.png |
| Árbol EXT3 final (491B) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_final_r2_tree.png |
| Superbloque EXT2 final | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_final_r3_sb.jpg |
| Superbloque EXT3 final | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_final_r4_sb.jpg |
| Árbol Sprint3 EXT2 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p3_final_r2_tree.png |
| LS permisos+propietario | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p3_final_r1_ls.jpg |

#### Comandos nuevos P2 (copy / move / remove / chmod / chown)
| Reporte | Link |
|---|---|
| Árbol después de copy | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_r1_tree_copy.png |
| Árbol después de move | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_r2_tree_move.png |
| Árbol después de remove | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_r3_tree_remove.png |
| LS chmod (permisos 755/600/111) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_r4_ls_chmod.jpg |
| LS chown (propietario user1) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_r5_ls_chown.jpg |
| Move dir completo (antes) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p3_move_r1_antes.png |
| Move dir completo (después) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p3_move_r2_despues.png |

#### EXT3 — 491B
| Reporte | Link |
|---|---|
| Superbloque EXT3 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_ext3_r1_sb.jpg |
| Inodos EXT3 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_ext3_r2_inode.jpg |
| Bloques EXT3 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_ext3_r3_block.jpg |
| Árbol EXT3 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_ext3_r4_tree.png |
| Bitmap inodos EXT3 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_ext3_r5_bm_inode.txt |
| Bitmap bloques EXT3 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_ext3_r6_bm_block.txt |

#### Sprint 3 — Disco2 (491D/492D) y Disco5 (491E)
| Reporte | Link |
|---|---|
| Árbol Disco2 (491D) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p3_d2_r1_tree.png |
| Superbloque Disco2 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p3_d2_r2_sb.jpg |
| Árbol Disco5 (491E) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p3_d5_r1_tree.png |
| Superbloque Disco5 | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p3_d5_r2_sb.jpg |

#### Inodos / bloques / superbloque EXT2 (491A)
| Reporte | Link |
|---|---|
| Inodos | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r1_inode.jpg |
| Bloques | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r2_block.jpg |
| Bitmap inodos | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r3_bm_inode.txt |
| Bitmap bloques | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r4_bm_block.txt |
| Superbloque | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r5_sb.jpg |
| Bloques de archivo | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r6_file.txt |
| LS directorio docs | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r7_ls.jpg |
| Árbol completo | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r8_tree.png |

#### FDISK (particiones eliminadas y ampliadas)
| Reporte | Link |
|---|---|
| Disco1 antes de delete | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_fdisk_r1_disk.jpg |
| MBR antes de delete | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_fdisk_r2_mbr.jpg |
| Disco1 después de add (+5MB) | http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p2_fdisk_r3_disk_add.jpg |

#### Discos — links directos (descargar .mia por SSH)
Los archivos `.mia` están en el EC2, no son accesibles por browser. Para verlos:
```bash
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@13.218.255.179 "ls -lh /home/ubuntu/Calificacion_MIA/Discos/"
# Disco1.mia  Disco2.mia  Disco3.mia  Disco4.mia  Disco5.mia
```

### 8. Comando `mounted` — resultado esperado al final
Pegar en el terminal del frontend:
```
mounted
```
Debe listar exactamente estas 7 particiones:
```
491A | Disco1.mia | Part11
492B | Disco3.mia | Part32
491B | Disco3.mia | Part31
491C | Disco4.mia | Part41
491D | Disco2.mia | Part21
492D | Disco2.mia | Part22
491E | Disco5.mia | Part51
```
> (El orden puede variar, lo importante es que estén todas.)

---

## Cómo cargar y ejecutar el script

1. Abrir el frontend.
2. En el panel del terminal (izquierda), clic en el ícono de carpeta **"Cargar archivo"**.
3. Seleccionar `Archivo_De_Prueba_P2.smia`.
4. El script aparece en el área de texto.
5. Clic en **"Ejecutar Script"** → el sistema ejecuta los 220 comandos en secuencia.

También puedes pegar comandos individuales en el input y presionar **"Ejecutar"** para pruebas puntuales.

---

## Checklist visual del frontend (antes de ejecutar cualquier script)

Al abrir el frontend por primera vez deberías ver:

- [ ] Header con el título **ExtreamFS**
- [ ] Barra de sesión: `Sin sesión activa` (gris/rojo)
- [ ] Botón **"Iniciar Sesión"** visible en la esquina superior derecha
- [ ] Panel terminal vacío a la izquierda
- [ ] Visualizador del sistema de archivos a la derecha (vacío, esperando particiones montadas)
- [ ] El backend responde: pegar `rep -id=X` cualquiera o simplemente notar que el botón de estado muestra conexión

---

## Fase 1 — Discos y particiones

**Script:** sección `SPRINT 1 - REVISION P1`

### Qué ejecutar
```
mkdisk (5 discos) → rmdisk (5 discos temporales) → fdisk → mount → mounted → rep (mbr, disk)
```

### Qué debe verse en el terminal
```
ERROR: Parámetros inválidos          ← mkdisk con param incorrecto (esperado)
OK: Disco creado correctamente ...   ← Disco1..Disco5
ERROR: El disco no existe ...        ← rmdisk DiscoN (esperado)
OK: Disco eliminado ...              ← Disco6..Disco10
ERROR: No se pudo abrir el disco     ← fdisk en DiscoN (esperado)
OK: Partición 'Part11' creada ...
OK: Partición 'Part12' creada ...
OK: Partición 'Part13' creada ...
OK: Partición 'Part14' creada ...
ERROR: Límite de 4 particiones ...   ← 5ta primaria (esperado)
ERROR: No hay espacio suficiente ... ← PartErr 20MB en disco de 13MB (esperado)
OK: Partición 'Part31' creada ...
OK: Partición 'Part32' creada ...
OK: Partición 'Part41' creada ...
OK: Partición montada con ID: 491A   ← Part11 en Disco1
OK: Partición montada con ID: 492A   ← Part12 en Disco1
ERROR: La partición ya está montada  ← doble mount de Part11 (esperado)
ERROR: Partición primaria 'Part0'... ← Part0 no existe en Disco3 (esperado)
OK: Partición montada con ID: 491B   ← Part31 en Disco3
OK: Partición montada con ID: 492B   ← Part32 en Disco3
OK: Partición montada con ID: 491C   ← Part41 en Disco4
```

El comando `mounted` debe listar exactamente: **491A, 492A, 491B, 492B, 491C**

Los reportes MBR y DISK para 491A y 491B deben responder:
```
OK: Reporte generado en /home/ubuntu/Calificacion_MIA/Reportes/p1_r1_disk.jpg
```

### Qué verificar en el Visualizador
- [ ] Clic en **Refrescar** → aparecen 3 discos: `Disco1.mia`, `Disco3.mia`, `Disco4.mia`
- [ ] Clic en `Disco1.mia` → aparecen `Part11 (491A)` y `Part12 (492A)`
- [ ] Clic en `Disco3.mia` → aparecen `Part31 (491B)` y `Part32 (492B)`
- [ ] Clic en `Disco4.mia` → aparece `Part41 (491C)`
- [ ] Las particiones aún no tienen filesystem — navegar dentro muestra carpeta vacía (normal)

---

## Fase 2 — Formateo EXT2 y EXT3

**Script:** sección `SPRINT 2 - MKFS EXT2 y EXT3`

### Qué debe verse en el terminal
```
OK: Archivo '/users.txt' creado correctamente
OK: MKFS realizado correctamente (EXT2)   ← 491A
OK: MKFS realizado correctamente (EXT2)   ← 492A
ERROR: ID no montado                      ← 499Z (esperado)
OK: MKFS realizado correctamente (EXT3)   ← 491B
OK: MKFS realizado correctamente (EXT3)   ← 492B
ERROR: ID no montado                      ← 211A (esperado, no existe ese ID)
```

### Qué verificar en el Visualizador
- [ ] Clic en `Part11 (491A)` → ahora se ve la carpeta raíz `/` con el archivo `users.txt`
- [ ] La carpeta `/` existe y se puede navegar (el filesystem ya existe)

---

## Fase 3 — Login y sesión

**Script:** sección `SISTEMA DE ARCHIVOS: USUARIOS Y DIRS`

### Opción A — Login por comando (en el terminal)
```
login -user=root -pass=123 -id=491A
```

### Opción B — Login por formulario (recomendado para mostrar la UI)
1. Clic en **"Iniciar Sesión"** (esquina superior derecha)
2. Llenar:
   - **ID Partición:** `491A`
   - **Usuario:** `root`
   - **Contraseña:** `123`
3. Clic en **Submit**

### Qué debe verse en el frontend después del login
- [ ] Barra de sesión muestra: `root | Grupo: root | Partición: 491A`
- [ ] Botón cambia a **"Cerrar Sesión"** (color diferente)
- [ ] El terminal responde: `OK: Sesión iniciada`

### Comandos de usuarios/grupos y qué esperar
| Comando | Resultado esperado |
|---|---|
| `login` doble | `ERROR: Ya hay una sesión activa` |
| `mkgrp -name=usuarios/adm/mail/news/sys` | `OK: Grupo 'X' creado con ID N` |
| `mkgrp -name=sys` (segunda vez) | `ERROR: El grupo 'sys' ya existe` |
| `cat -file1=/users.txt` | muestra contenido del archivo de usuarios |
| `rmgrp -name=mail` | `OK: Grupo 'mail' eliminado` |
| `mkusr -user=usuario1/user1/user2` | `OK: Usuario 'X' creado con ID N` |
| `mkusr -user=user2` (segunda vez) | `ERROR: El usuario 'user2' ya existe` |
| `mkusr -user=user3 -grp=system` | `ERROR: El grupo 'system' no existe` |
| `chgrp -user=user2 -grp=adm` | `OK: Usuario 'user2' movido al grupo 'adm'` |
| `rmusr -user=user2` | `OK: Usuario 'user2' eliminado` |
| `logout` doble | segundo: `ERROR: No hay sesión activa` |

---

## Fase 4 — Directorios y archivos

**Script:** continuación después del login de root en 491A

### Qué debe verse en el terminal
```
OK: Carpeta '/bin' creada correctamente
ERROR: La carpeta padre 'home' no existe    ← mkdir sin -p (esperado)
OK: Carpeta '/home/archivos/user/docs/usac' creada correctamente   ← mkdir -p
OK: Carpeta '/home/archivos/carpeta1/.../carpeta5' creada ...
OK: Sesión de 'root' cerrada correctamente
OK: Sesión iniciada                          ← login user1
OK: Sesión de 'user1' cerrada correctamente
OK: Sesión iniciada                          ← login root de nuevo
OK: Archivo '/home/archivos/user/docs/Tarea.txt' creado (75 bytes)
OK: Archivo '/home/archivos/user/docs/Tarea2.txt' creado (768 bytes)
OK: Archivo '/home/archivos/user/docs/Tarea3.txt' creado (con contenido)
ERROR: La carpeta 'noexiste' no existe       ← mkfile ruta mala (esperado)
ERROR: -size no puede ser negativo           ← size=-45 (esperado)
OK: Archivo '.../fase1/entrada.txt' creado   ← mkfile -r recursivo
```

El `cat` sobre `Tarea3.txt` debe mostrar:
```
Contenido de prueba para Tarea3 MIA Proyecto 2
```

### Qué verificar en el Visualizador
- [ ] Refrescar → navegar `491A` → `/` → `home` → `archivos` → `user` → `docs`
- [ ] Se ven los archivos: `Tarea.txt`, `Tarea2.txt`, `Tarea3.txt`, y la carpeta `usac`
- [ ] Clic en `Tarea3.txt` → panel derecho muestra el contenido del archivo
- [ ] Ruta en breadcrumb muestra: `/home/archivos/user/docs/Tarea3.txt`

### Reportes generados (verificar respuesta OK)
- `p4_r1_inode.jpg` — inodos
- `p4_r2_block.jpg` — bloques
- `p4_r3_bm_inode.txt` — bitmap inodos
- `p4_r4_bm_block.txt` — bitmap bloques
- `p4_r5_sb.jpg` — superbloque
- `p4_r6_file.txt` — bloques del archivo Tarea2.txt
- `p4_r7_ls.jpg` — listado de /home/archivos/user/docs
- `p4_r8_tree.png` — árbol completo

---

## Fase 5 — Comandos nuevos P2 (sobre 491A EXT2)

**Script:** sección `SPRINT 2 - COMANDOS NUEVOS P2`

### rename
```
OK: '/home/archivos/user/docs/Tarea.txt' renombrado a 'TareaRenombrada.txt'
ERROR: La ruta '.../NoExiste.txt' no existe    ← esperado
```
- [ ] Visualizador: en `docs` ya no está `Tarea.txt`, aparece `TareaRenombrada.txt`
- [ ] `cat -file1=.../TareaRenombrada.txt` muestra contenido del archivo

### copy
```
OK: '/home/archivos/user/docs/Tarea2.txt' copiado a '/home/archivos/carpeta1'
OK: '/home/archivos/user/docs/usac' copiado a '/home/archivos/carpeta1/carpeta2'
ERROR: La carpeta destino '.../noexiste' no existe    ← esperado
```
- [ ] Visualizador + reporte `p2_r1_tree_copy.png`: `Tarea2.txt` aparece en `carpeta1`; carpeta `usac` copiada dentro de `carpeta2`

### move
```
OK: '.../TareaRenombrada.txt' movido a '/home/archivos/carpeta1'
ERROR: La carpeta destino '.../noexiste' no existe    ← esperado
```
- [ ] Visualizador + reporte `p2_r2_tree_move.png`: `TareaRenombrada.txt` desaparece de `docs` y aparece en `carpeta1`

### remove
```
OK: '/home/archivos/user/docs/Tarea2.txt' eliminado
OK: '/home/archivos/carpeta1/carpeta2/carpeta3' eliminado    ← recursivo
ERROR: La ruta '.../NoExiste.txt' no existe    ← esperado
```
- [ ] Visualizador + reporte `p2_r3_tree_remove.png`: `Tarea2.txt` ya no está en `docs`; `carpeta3` y todo su contenido eliminados

### find
El `find` muestra resultados en el terminal (sin prefijo OK:), no en el visualizador.
```
find -path=/home -name=*.txt       → lista todos los .txt bajo /home
find -path=/home/.../docs -name=Tarea?.txt  → Tarea3.txt (patrón con ?)
find -path=/ -name=entrada.txt     → /home/archivos/user/docs/usac/.../entrada.txt
ERROR: La ruta '/noexiste' no existe    ← esperado
```

### chmod
```
OK: Permisos de '.../Tarea3.txt' cambiados a 755
OK: Permisos de '/home/archivos/user' cambiados a 775 (recursivo)
ERROR: Cada dígito de -ugo debe estar entre 0 y 7    ← ugo=999 (esperado)
```
- [ ] Reporte `p2_r4_ls_chmod.jpg`: columna de permisos muestra `rwxr-xr-x` para Tarea3.txt

### chown
```
OK: Propietario de '.../Tarea3.txt' cambiado a 'user1'
OK: Propietario de '/home/archivos/user' cambiado a 'user1' (recursivo)
ERROR: El usuario 'noexiste' no existe    ← esperado
```
- [ ] Reporte `p2_r5_ls_chown.jpg`: columna de propietario muestra `user1`

---

## Fase 6 — EXT3 y Journaling (sobre 491B)

**Script:** sección `SPRINT 2 - EXT3 y JOURNALING`

### Qué debe verse en el terminal
```
OK: Sesión iniciada    ← login root en 491B
OK: Carpeta '/home/user1/documentos' creada
OK: Carpeta '/home/user1/proyectos/mia' creada
OK: Archivo '.../nota.txt' creado (50 bytes)
OK: Archivo '.../readme.md' creado (120 bytes)
OK: Archivo '.../main.cpp' creado (200 bytes)
OK: '.../nota.txt' renombrado a 'nota_v2.txt'
OK: '.../readme.md' copiado a .../proyectos/mia
OK: '.../nota_v2.txt' movido a .../proyectos
OK: '.../readme.md' eliminado
OK: Permisos de '.../main.cpp' cambiados a 700
OK: Propietario de '.../proyectos' cambiado a 'user1' (recursivo)
```

Después de `journaling -id=491B`:
```
Journal 491B:
[1] mkdir  /home/user1/documentos      2026-...
[2] mkdir  /home/user1/proyectos       2026-...
[3] mkfile /home/user1/documentos/nota.txt    ...
[4] rename /home/user1/documentos/nota.txt    ...
[5] copy   /home/user1/documentos/readme.md   ...
[6] move   /home/user1/documentos/nota_v2.txt ...
[7] remove /home/user1/proyectos/mia/readme.md ...
[8] chmod  /home/user1/proyectos/mia/main.cpp ...
[9] chown  /home/user1/proyectos ...
```

Después de `loss -id=491B`:
```
OK: Sistema de archivos EXT3 en '491B' simulado como perdido
```

El segundo `journaling -id=491B` después del `loss` muestra el journal vacío o reiniciado — confirma la recuperación.

### Qué verificar en el Visualizador — Journal
1. Navegar a `Disco3.mia` → `Part31 (491B)`
2. Ir a cualquier carpeta (ej. `/home/user1/proyectos`)
- [ ] Clic en botón **Journal** en la barra de ruta o panel
- [ ] Se despliega la lista de operaciones: mkdir, mkfile, rename, copy, move, remove, chmod, chown
- [ ] Cada entrada muestra operación, ruta e inodo afectado

### Reportes EXT3 (todos deben responder OK)
- `p2_ext3_r1_sb.jpg` — superbloque de 491B
- `p2_ext3_r2_inode.jpg` — inodos
- `p2_ext3_r3_block.jpg` — bloques
- `p2_ext3_r4_tree.png` — árbol
- `p2_ext3_r5_bm_inode.txt` — bitmap
- `p2_ext3_r6_bm_block.txt` — bitmap

---

## Fase 7 — FDISK DELETE y ADD

**Script:** sección `SPRINT 2 - FDISK ADD/DELETE`

### Qué debe verse en el terminal
```
OK: Partición '492A' desmontada correctamente
ERROR: ID '499Z' no está montado    ← esperado
--- PARTICIONES MONTADAS ---        ← mounted: 491A, 491B, 492B, 491C
OK: Partición 'Part13' eliminada (fast)
OK: Partición 'Part14' eliminada (full)
ERROR: La partición 'Part11' está montada. Desmóntela antes    ← esperado
OK: Reporte generado ...p2_fdisk_r1_disk.jpg
OK: Reporte generado ...p2_fdisk_r2_mbr.jpg
OK: Partición 'Part11' ampliada en 5242880 bytes    ← +5MB
ERROR: No hay espacio libre suficiente ...           ← +100MB en Part12 (esperado)
OK: Reporte generado ...p2_fdisk_r3_disk_add.jpg
```

### Qué verificar en el Visualizador
- [ ] Refrescar → `Disco1.mia` ahora solo muestra `Part11 (491A)` (Part12 desmontada, Part13 y Part14 eliminadas)
- [ ] El reporte `disk` debe mostrar `Part11` más grande (ahora 15MB)

---

## Fase 8 — Sprint 3 (pruebas adicionales)

**Script:** sección `SPRINT 3 - PRUEBAS ADICIONALES`

### A) cat con múltiples archivos
```
cat -file1=Tarea3.txt -file2=entrada.txt          → contenido de ambos concatenados
cat -file1=Tarea3.txt -file2=entrada.txt -file3=Tarea2.txt  → tres archivos
ERROR: El archivo '.../noexiste.txt' no existe     ← esperado
```

### B) Disco2 y Disco5 nuevos
IDs asignados en orden: **491D** (Disco2/Part21), **492D** (Disco2/Part22), **491E** (Disco5/Part51 EXT3)

```
OK: Partición 'Part21' creada correctamente
OK: Partición 'Part22' creada correctamente
ERROR: No hay espacio suficiente ...    ← Part2Err 30MB (esperado)
OK: Partición montada con ID: 491D
OK: Partición montada con ID: 492D
OK: MKFS realizado correctamente (EXT2)   ← 491D
OK: MKFS realizado correctamente (EXT2)   ← 492D
OK: Partición montada con ID: 491E
OK: MKFS realizado correctamente (EXT3)   ← 491E
```
- [ ] Visualizador muestra ahora `Disco2.mia` y `Disco5.mia`
- [ ] Navegando `491D` aparece `/home/disco2/subdir/archivo.txt`
- [ ] `journaling -id=491E` muestra las operaciones de Disco5

### C) Sesión de usuario1
```
OK: Sesión iniciada    ← user1 en 491A
(cat, mkfile, find como user1)
OK: Sesión de 'user1' cerrada correctamente
```
- [ ] Barra de sesión muestra: `user1 | Grupo: usuarios | Partición: 491A`

### D) Move de directorio completo
```
OK: Carpeta '/home/archivos/dirOrigen/subA/subB' creada
OK: '/home/archivos/dirOrigen' movido a '/home/archivos/carpeta1/carpeta2'
ERROR: La ruta '/home/archivos/noExiste' no existe    ← esperado
```
- [ ] Reporte `p3_move_r2_despues.png`: `dirOrigen` ya no está en `/home/archivos`, aparece dentro de `carpeta2`

### E) find con más patrones
```
find -path=/ -name=*.md        → readme.md en 491B
find -path=/home -name=Tarea*  → TareaRenombrada.txt, Tarea3.txt...
find -path=/ -name=docs        → /home/archivos/user/docs
ERROR: La ruta '/ruta/invalida' no existe    ← esperado
```

### F) mkfile con -cont y G) chmod/chown edge cases
```
OK: Archivo '.../ConContenido.txt' creado
(cat muestra el contenido del archivo NAME.txt)
OK: Permisos de '.../MiArchivo.txt' cambiados a 600
OK: Permisos de '.../ConContenido.txt' cambiados a 111
OK: Propietario de '.../ConContenido.txt' cambiado a 'usuario1'
```
- [ ] Reporte `p3_final_r1_ls.jpg`: permisos `rw-------` (600) y `--x--x--x` (111) visibles

---

## Resumen final (al final del script)

```
mounted → lista todas las particiones: 491A, 491B, 492B, 491C, 491D, 492D, 491E
```
- [ ] Reporte `p2_final_r1_tree.png` (491A): árbol completo del sistema EXT2
- [ ] Reporte `p2_final_r2_tree.png` (491B): árbol del sistema EXT3
- [ ] Reporte `p2_final_r3_sb.jpg` (491A): superbloque EXT2
- [ ] Reporte `p2_final_r4_sb.jpg` (491B): superbloque EXT3

---

## Cómo ver los reportes generados

Los reportes `.jpg` / `.png` se guardan en el EC2 en `/home/ubuntu/Calificacion_MIA/Reportes/`. Hay dos formas de verlos:

### Opción 1 — Desde el frontend (reporte individual)
Si el visualizador tiene soporte para mostrar reportes, la URL directa es:
```
http://13.218.255.179:8080/report?path=/home/ubuntu/Calificacion_MIA/Reportes/p4_r8_tree.png
```
Pegar esa URL en el navegador para ver la imagen generada.

### Opción 2 — Descargar por SCP desde tu máquina
```bash
scp -i ~/.ssh/extreamfs-key.pem \
  ubuntu@13.218.255.179:~/Calificacion_MIA/Reportes/* \
  ~/Desktop/reportes_mia/
```

---

## Errores esperados (no son bugs)

| Comando | Error que aparece | Por qué es correcto |
|---|---|---|
| `mkdisk -param=x` | `ERROR: Parámetros inválidos` | Rechaza parámetros desconocidos |
| `rmdisk DiscoN.mia` | `ERROR: El disco no existe` | El archivo no existe |
| `fdisk` en DiscoN | `ERROR: No se pudo abrir el disco` | No puede abrir disco inexistente |
| `fdisk` 5ta primaria en Disco1 | `ERROR: Límite de 4 particiones` | MBR solo admite 4 primarias |
| `fdisk` 20MB en disco de 13MB | `ERROR: No hay espacio suficiente` | No cabe en el disco |
| `mount` Part0 en Disco3 | `ERROR: Partición primaria 'Part0' no encontrada` | No existe esa partición |
| `mount` Part11 segunda vez | `ERROR: La partición ya está montada` | No se monta dos veces |
| `mkfs -id=499Z` | `ERROR: ID no montado` | ID no asignado |
| `mkfs -id=211A` | `ERROR: ID no montado` | Part41 recibió ID 491C, no 211A |
| `login` doble | `ERROR: Ya hay una sesión activa` | Una sesión a la vez |
| `logout` doble | `ERROR: No hay sesión activa` | No hay sesión que cerrar |
| `mkgrp -name=sys` segunda vez | `ERROR: El grupo 'sys' ya existe` | No permite duplicados |
| `mkusr -grp=system` | `ERROR: El grupo 'system' no existe` | El grupo no fue creado |
| `mkusr -user=user2` segunda vez | `ERROR: El usuario 'user2' ya existe` | No permite duplicados |
| `mkdir /home/archivos` sin -p | `ERROR: La carpeta padre 'home' no existe` | Sin -p no crea intermedios |
| `mkfile -path=.../noexiste/...` | `ERROR: La carpeta 'noexiste' no existe` | La ruta padre no existe |
| `mkfile -size=-45` | `ERROR: -size no puede ser negativo` | Tamaño inválido |
| `rename` ruta inexistente | `ERROR: La ruta '...' no existe` | El archivo no existe |
| `copy` / `move` destino malo | `ERROR: La carpeta destino '...' no existe` | Destino no existe |
| `find` ruta inválida | `ERROR: La ruta '...' no existe` | Path de búsqueda no existe |
| `chmod -ugo=999` | `ERROR: Cada dígito debe estar entre 0 y 7` | 9 no es octal válido |
| `chown -usuario=noexiste` | `ERROR: El usuario 'noexiste' no existe` | Usuario no registrado |
| `unmount -id=499Z` | `ERROR: ID '499Z' no está montado` | El ID no existe |
| `fdisk -delete` en Part11 montada | `ERROR: La partición 'Part11' está montada` | Hay que desmontar primero |
| `fdisk -add=100M` en Part12 | `ERROR: No hay espacio libre suficiente` | Solo hay ~30MB libres |
| `fdisk` Part2Err 30MB en Disco2 | `ERROR: No hay espacio suficiente` | 35MB ya usados de 50MB |

---

## Flujo de repaso rápido

```
1. Abrir frontend → confirmar barra de sesión inactiva
2. Cargar Archivo_De_Prueba_P2.smia → clic Ejecutar Script
3. Mientras ejecuta: refrescar el Visualizador → confirmar que aparecen los íconos de disco
4. Navegar: Disco1 → 491A → /home/archivos/user/docs → clic en Tarea3.txt
   → confirmar que el contenido aparece en el panel derecho
5. Cuando llegue la parte de login en el script:
   → barra de sesión cambia a: root | root | 491A
6. También probar el Login por formulario: botón "Iniciar Sesión" → llenar form manualmente
7. Navegar el árbol de carpetas mientras el script corre
8. Sección EXT3: ir a Disco3 → 491B → botón Journal
   → revisar que las operaciones registradas aparecen correctamente
9. Al final: mounted lista 7 particiones (491A, 491B, 492B, 491C, 491D, 492D, 491E)
10. Logout desde el botón → barra de sesión vuelve a inactiva
```
