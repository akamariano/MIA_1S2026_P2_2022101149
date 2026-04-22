# Manual Técnico — ExtreamFS

| Nombre | Carnet |
|---|---|
| Mariano Roberto Rac Noguera | 202101149 |

---

## Tabla de Contenidos

1. [Descripción General](#descripción-general)
2. [Arquitectura del Sistema](#arquitectura-del-sistema)
3. [Despliegue en AWS](#despliegue-en-aws)
4. [Estructura de Carpetas](#estructura-de-carpetas)
5. [Tecnologías Utilizadas](#tecnologías-utilizadas)
6. [Estructuras de Datos](#estructuras-de-datos)
7. [Sistema de Archivos EXT2](#sistema-de-archivos-ext2)
8. [Sistema de Archivos EXT3 y Journaling](#sistema-de-archivos-ext3-y-journaling)
9. [Comandos Implementados](#comandos-implementados)
10. [API REST del Backend](#api-rest-del-backend)
11. [Flujo de Ejecución](#flujo-de-ejecución)
12. [Compilación](#compilación)

---

## Descripción General

**ExtreamFS** es un simulador de sistemas de archivos EXT2 y EXT3 desarrollado como Proyecto 2 del curso Manejo e Implementación de Archivos (MIA), 1S 2026, FIUSAC.

El sistema permite:
- Crear y gestionar discos virtuales (archivos `.mia`)
- Particionar discos con tabla MBR (primarias, extendidas y lógicas)
- Formatear particiones con EXT2 o EXT3
- Gestionar usuarios, grupos y permisos POSIX
- Ejecutar operaciones sobre el sistema de archivos (crear, copiar, mover, eliminar, renombrar, buscar)
- Simular pérdida y recuperación de datos EXT3 (journaling)
- Visualizar reportes gráficos (Graphviz) de la estructura interna
- Acceder a todo lo anterior desde una interfaz web desplegada en AWS

---

## Arquitectura del Sistema

```
┌──────────────────────────────────────────────────────┐
│                     USUARIO                          │
└──────────────────┬───────────────────────────────────┘
                   │ HTTPS
┌──────────────────▼───────────────────────────────────┐
│          Frontend — AWS S3 (sitio web estático)       │
│  Next.js + TypeScript + Tailwind CSS                  │
│  http://extreamfs-frontend-849279003367               │
│       .s3-website-us-east-1.amazonaws.com             │
└──────────────────┬───────────────────────────────────┘
                   │ HTTP REST API (puerto 8080)
┌──────────────────▼───────────────────────────────────┐
│          Backend — AWS EC2 t3.micro (Ubuntu 22.04)    │
│  C++ + cpp-httplib                                    │
│  Elastic IP: 13.218.255.179                           │
│  Servicio systemd: extreamfs                          │
└──────────────────┬───────────────────────────────────┘
                   │ Archivos binarios .mia
┌──────────────────▼───────────────────────────────────┐
│      Discos virtuales en el servidor EC2              │
│  /home/ubuntu/Calificacion_MIA/Discos/                │
│  Disco1.mia … Disco5.mia (EXT2 / EXT3)               │
└──────────────────────────────────────────────────────┘
```

### Componentes del Frontend

| Componente | Archivo | Función |
|---|---|---|
| Página principal | `app/page.tsx` | Layout con terminal y explorador |
| Panel de comandos | `components/CommandPanel.tsx` | Input de comandos, carga de scripts |
| Explorador de archivos | `components/FileExplorer.tsx` | Navegación disco→partición→carpeta→archivo |
| Barra de sesión | `components/SessionBar.tsx` | Estado de login, botón iniciar/cerrar sesión |
| Visor de bloques | `components/BlockViewer.tsx` | Contenido de archivos y journal |
| Cliente HTTP | `services/api.ts` | Todas las llamadas al backend |

### Componentes del Backend

| Módulo | Ruta | Función |
|---|---|---|
| Servidor HTTP | `core/server/server.cpp` | Recibe peticiones, enruta comandos |
| Comandos | `core/commands/` | ~42 handlers de comandos |
| Filesystem EXT2/3 | `core/filesystem/` | Lectura/escritura de inodos y bloques |
| Journaling | `core/filesystem/journal_manager.cpp` | Registro de operaciones EXT3 |
| Montaje | `core/mount/mount_manager.cpp` | Estado en memoria de particiones montadas |
| Disco | `core/disk/` | Creación y particionamiento de discos |
| Reportes | `core/reports/` | Generación de grafos con Graphviz |

---

## Despliegue en AWS

### Infraestructura

| Servicio | Configuración | URL |
|---|---|---|
| **EC2** | t3.micro, Ubuntu 22.04, us-east-1 | `13.218.255.179:8080` |
| **S3** | Sitio web estático, acceso público | [Link del frontend](http://extreamfs-frontend-849279003367.s3-website-us-east-1.amazonaws.com) |
| **Elastic IP** | `13.218.255.179` — IP fija, no cambia al reiniciar | — |

### Security Groups del EC2

| Puerto | Protocolo | Origen | Propósito |
|---|---|---|---|
| 22 | TCP | 0.0.0.0/0 | SSH para administración |
| 8080 | TCP | 0.0.0.0/0 | API REST del backend |

### Proceso de despliegue

**Backend (EC2):**
```bash
# Compilar localmente
cd frontend/backend && cmake -B build && cd build && make -j$(nproc)

# Copiar al servidor
scp -i ~/.ssh/extreamfs-key.pem extreamfs ubuntu@13.218.255.179:~/extreamfs/

# Reiniciar servicio
ssh -i ~/.ssh/extreamfs-key.pem ubuntu@13.218.255.179 "sudo systemctl restart extreamfs"
```

**Frontend (S3):**
```bash
cd frontend
npm run build
aws s3 sync out/ s3://extreamfs-frontend-849279003367/ --delete --region us-east-1
```

### Servicio systemd del backend

```ini
[Unit]
Description=ExtreamFS Backend
After=network.target

[Service]
ExecStart=/home/ubuntu/extreamfs/extreamfs
WorkingDirectory=/home/ubuntu/extreamfs
Restart=always
User=ubuntu

[Install]
WantedBy=multi-user.target
```

---

## Estructura de Carpetas

```
MIA_1S2026_P2_202101149/
├── frontend/
│   ├── backend/                        # Backend C++
│   │   ├── CMakeLists.txt
│   │   └── core/
│   │       ├── commands/               # Handlers de comandos
│   │       │   ├── mkdisk.cpp/h        # Crear disco
│   │       │   ├── rmdisk.cpp/h        # Eliminar disco
│   │       │   ├── fdisk.cpp/h         # Particionar (create/delete/add)
│   │       │   ├── mount.cpp/h         # Montar partición
│   │       │   ├── unmount_cmd.cpp/h   # Desmontar partición
│   │       │   ├── mkfs.cpp/h          # Formatear EXT2/EXT3
│   │       │   ├── mkdir_cmd.cpp/h     # Crear directorio
│   │       │   ├── mkfile_cmd.cpp/h    # Crear archivo
│   │       │   ├── cat_cmd.cpp/h       # Leer archivo(s)
│   │       │   ├── rename_cmd.cpp/h    # Renombrar
│   │       │   ├── copy_cmd.cpp/h      # Copiar archivo/dir
│   │       │   ├── move_cmd.cpp/h      # Mover archivo/dir
│   │       │   ├── remove_cmd.cpp/h    # Eliminar archivo/dir
│   │       │   ├── find_cmd.cpp/h      # Buscar por nombre
│   │       │   ├── chmod_cmd.cpp/h     # Cambiar permisos
│   │       │   ├── chown_cmd.cpp/h     # Cambiar propietario
│   │       │   ├── journaling_cmd.cpp/h# Ver journal EXT3
│   │       │   ├── loss_cmd.cpp/h      # Simular pérdida EXT3
│   │       │   ├── mkusr.cpp/h         # Crear usuario
│   │       │   ├── rmusr.cpp/h         # Eliminar usuario
│   │       │   ├── mkgrp.cpp/h         # Crear grupo
│   │       │   ├── rmgrp.cpp/h         # Eliminar grupo
│   │       │   └── chgrp.cpp/h         # Cambiar grupo de usuario
│   │       ├── filesystem/             # EXT2/EXT3 internals
│   │       │   ├── ext2_reader.cpp/h
│   │       │   ├── ext2_writer.cpp/h
│   │       │   ├── SuperBlock.h
│   │       │   ├── Inode.h
│   │       │   ├── Blocks.h
│   │       │   ├── journal_manager.cpp/h
│   │       │   ├── session_manager.cpp/h
│   │       │   └── permissions.h
│   │       ├── disk/                   # MBR / EBR
│   │       ├── mount/                  # Mount manager
│   │       ├── reports/                # Graphviz report generators
│   │       └── server/                 # cpp-httplib HTTP server
│   ├── app/                            # Next.js App Router
│   ├── components/                     # React components
│   ├── services/api.ts                 # HTTP client
│   ├── .env.production                 # NEXT_PUBLIC_API_URL
│   └── .env.development.local          # localhost:8080
├── Documentacion/
│   ├── TECHNICAL_GUIDE.md              # Este documento
│   ├── USER_GUIDE.md                   # Manual de usuario
│   ├── AWS_GUIA_RAPIDA.md              # Referencia de operaciones AWS
│   └── GUIA_DE_PRUEBAS.md             # Checklist de pruebas
└── Archivo_De_Prueba_P2.smia           # Script de pruebas (221 comandos)
```

---

## Tecnologías Utilizadas

### Backend
| Tecnología | Propósito |
|---|---|
| C++17 | Lenguaje principal |
| CMake 3.22+ | Sistema de compilación |
| cpp-httplib | Servidor HTTP embebido |
| Graphviz (`dot`) | Generación de reportes gráficos |

### Frontend
| Tecnología | Propósito |
|---|---|
| Next.js 15 | Framework React (App Router) |
| TypeScript | Tipado estático |
| Tailwind CSS 4 | Estilos |

### Infraestructura
| Servicio | Propósito |
|---|---|
| AWS EC2 t3.micro | Servidor backend C++ (Ubuntu 22.04) |
| AWS S3 | Hosting del frontend estático |
| AWS Elastic IP | IP fija para el backend |
| systemd | Auto-restart del servicio backend |

---

## Estructuras de Datos

### MBR (Master Boot Record)

```cpp
struct MBR {
    long mbr_size;               // Tamaño total del disco en bytes
    char mbr_fecha[19];          // Fecha de creación
    char mbr_fit;                // Algoritmo de ajuste: B/F/W
    Partition mbr_partitions[4]; // Tabla de 4 particiones
};

struct Partition {
    char part_status;   // '0'=inactiva, '1'=activa
    char part_type;     // 'P'=primaria, 'E'=extendida, 'L'=lógica
    char part_fit;      // 'B'=BestFit, 'F'=FirstFit, 'W'=WorstFit
    long part_start;    // Offset en bytes desde inicio del disco
    long part_size;     // Tamaño en bytes
    char part_name[16]; // Nombre de la partición
    char part_correlative[4]; // Correlativo para ID de montaje
    char part_id[4];    // ID asignado al montar (ej: 491A)
};
```

### EBR (Extended Boot Record)

```cpp
struct EBR {
    char part_mount;    // '0' no montada, '1' montada
    char part_fit;
    long part_start;
    long part_size;
    long part_next;     // Offset del siguiente EBR (-1 si es el último)
    char part_name[16];
};
```

### SuperBloque EXT2/EXT3

```cpp
struct SuperBlock {
    int  s_filesystem_type;   // 2=EXT2, 3=EXT3
    int  s_inodes_count;      // Total de inodos
    int  s_blocks_count;      // Total de bloques
    int  s_free_blocks_count;
    int  s_free_inodes_count;
    int  s_mtime;             // Última montada
    int  s_umtime;            // Última desmontada
    int  s_mnt_count;         // Veces montada
    int  s_magic;             // 0xEF53
    int  s_inode_size;        // sizeof(Inode)
    int  s_block_size;        // sizeof(DirectoryBlock/etc)
    int  s_first_ino;         // Primer inodo disponible
    long s_first_blo;         // Primer bloque disponible
    long s_bm_inode_start;    // Inicio bitmap de inodos
    long s_bm_block_start;    // Inicio bitmap de bloques
    long s_inode_start;       // Inicio tabla de inodos
    long s_block_start;       // Inicio área de bloques
};
```

### Inodo

```cpp
struct Inode {
    int  i_uid;          // UID del propietario
    int  i_gid;          // GID del grupo
    int  i_size;         // Tamaño en bytes
    char i_atime[19];    // Último acceso
    char i_ctime[19];    // Creación
    char i_mtime[19];    // Última modificación
    int  i_block[15];    // Punteros a bloques (12 directos + 3 indirectos)
    char i_type;         // '0'=carpeta, '1'=archivo
    int  i_perm;         // Permisos (ej: 664, 755)
};
```

### Bloques

```cpp
// Bloque de directorio
struct DirectoryBlock {
    DirectoryContent b_content[4]; // 4 entradas por bloque
};

struct DirectoryContent {
    int  b_inodo;    // Número de inodo (-1=vacío)
    char b_name[12]; // Nombre del archivo/carpeta
};

// Bloque de archivo (contenido)
struct FileBlock {
    char b_content[64]; // 64 bytes de contenido
};

// Bloque de punteros (indirecto)
struct PointerBlock {
    int b_pointers[16]; // 16 punteros a bloques
};
```

---

## Sistema de Archivos EXT2

### Layout en disco

```
[part_start]
├── SuperBloque         (sizeof SuperBlock bytes)
├── Bitmap de Inodos    (s_inodes_count bytes, 1 bit por inodo)
├── Bitmap de Bloques   (s_blocks_count bytes, 1 bit por bloque)
├── Tabla de Inodos     (s_inodes_count × sizeof Inode bytes)
└── Área de Bloques     (s_blocks_count × 64 bytes)
    ├── Bloque 0: raíz '/' (DirectoryBlock)
    └── Bloques 1..N: datos
```

### Número de estructuras

```
tamaño_particion = sizeof(SuperBlock)
                 + n × sizeof(Inode)
                 + n              (bitmap inodos)
                 + 3×n × 64      (bloques: dir + archivo + contenido)
                 + 3×n           (bitmap bloques)

n = floor(resultado de despejar n)
```

### Inodo raíz

Al formatear con `mkfs`, se crean automáticamente:
- Inodo 0 → directorio raíz `/`
- Inodo 1 → archivo `/users.txt` (usuarios y grupos del sistema)

### ID de montaje

El ID sigue el formato `{contador}{disco_letra}{partición_correlativo}`:
- `4` = número de disco (49 = número de disco en el sistema)
- `91A` = primer disco letra A, partición 1

Ejemplo: `491A` = primera partición montada del primer disco del sistema 49.

---

## Sistema de Archivos EXT3 y Journaling

### Diferencia con EXT2

EXT3 agrega un área de **journaling** entre el SuperBloque y el Bitmap de Inodos:

```
[part_start]
├── SuperBloque
├── Área de Journaling  (50 × sizeof(Journal) bytes)   ← NUEVO en EXT3
├── Bitmap de Inodos
├── Bitmap de Bloques
├── Tabla de Inodos
└── Área de Bloques
```

### Estructura Journal

```cpp
struct Journal {
    int j_count;           // Número de la entrada (-1 = vacía)
    Information j_content; // Contenido de la operación
};

struct Information {
    char i_operation[10]; // Operación: mkdir, mkfile, rename, copy, move, remove, chmod, chown
    char i_path[32];      // Ruta afectada
    char i_content[64];   // Contenido adicional (nombre nuevo, usuario, permisos, etc.)
    char i_date[19];      // Fecha y hora de la operación
};
```

### Operaciones que generan entrada en el journal

| Operación | i_operation | i_path | i_content |
|---|---|---|---|
| `mkdir` | `"mkdir"` | ruta del directorio | — |
| `mkfile` | `"mkfile"` | ruta del archivo | — |
| `rename` | `"rename"` | ruta original | nuevo nombre |
| `copy` | `"copy"` | ruta origen | destino |
| `move` | `"move"` | ruta origen | destino |
| `remove` | `"remove"` | ruta eliminada | — |
| `chmod` | `"chmod"` | ruta del archivo | nuevo permiso (ej: "755") |
| `chown` | `"chown"` | ruta del archivo | nuevo usuario |

### Comando `loss` — Simulación de pérdida

El comando `loss -id=491B` simula una pérdida del sistema de archivos EXT3 borrando el área de journaling (sobreescribiendo con zeros). Después del `loss`, el journal queda vacío y el sistema puede continuar operando (simulación de recuperación).

---

## Comandos Implementados

### Gestión de discos

| Comando | Parámetros obligatorios | Parámetros opcionales | Descripción |
|---|---|---|---|
| `mkdisk` | `-size=N -path=ruta` | `-unit=B/K/M -fit=BF/FF/WF` | Crea un disco virtual `.mia` |
| `rmdisk` | `-path=ruta` | — | Elimina el archivo de disco |

### Particionamiento

| Comando | Parámetros | Descripción |
|---|---|---|
| `fdisk` (crear) | `-name=X -size=N -path=ruta` | `-type=P/E/L -unit=B/K/M -fit=BF/FF/WF` | Crea partición |
| `fdisk` (eliminar) | `-delete=fast/full -name=X -path=ruta` | — | `fast`: marca vacía, `full`: rellena con \0 |
| `fdisk` (resize) | `-add=N -name=X -path=ruta` | `-unit=B/K/M` | N positivo = ampliar, N negativo = reducir |
| `mount` | `-path=ruta -name=X` | — | Monta una partición primaria |
| `unmount` | `-id=ID` | — | Desmonta por ID |
| `mounted` | — | — | Lista todas las particiones montadas |

### Formateo

| Comando | Parámetros obligatorios | Parámetros opcionales | Descripción |
|---|---|---|---|
| `mkfs` | `-id=ID` | `-type=full -fs=2fs/3fs` | Formatea como EXT2 (default) o EXT3 |

### Sesión

| Comando | Parámetros | Descripción |
|---|---|---|
| `login` | `-user=X -pass=X -id=ID` | Inicia sesión en una partición |
| `logout` | — | Cierra la sesión activa |

### Usuarios y grupos

| Comando | Parámetros | Descripción |
|---|---|---|
| `mkgrp` | `-name=X` | Crea un grupo (solo root) |
| `rmgrp` | `-name=X` | Elimina un grupo (solo root) |
| `mkusr` | `-user=X -pass=X -grp=X` | Crea un usuario (solo root) |
| `rmusr` | `-user=X` | Elimina un usuario (solo root) |
| `chgrp` | `-user=X -grp=X` | Cambia el grupo de un usuario (solo root) |

### Operaciones de archivos

| Comando | Parámetros obligatorios | Parámetros opcionales | Descripción |
|---|---|---|---|
| `mkdir` | `-path=ruta` | `-p` (crea padres), `-r` (recursivo) | Crea directorio |
| `mkfile` | `-path=ruta` | `-size=N -cont=ruta -r` | Crea archivo con contenido opcional |
| `cat` | `-file1=ruta` | `-file2=ruta -file3=ruta` | Lee y concatena hasta 3 archivos |
| `rename` | `-path=ruta -name=nuevo` | — | Renombra archivo o carpeta |
| `copy` | `-path=ruta -destino=ruta` | — | Copia archivo o directorio completo |
| `move` | `-path=ruta -destino=ruta` | — | Mueve archivo o directorio completo |
| `remove` | `-path=ruta` | — | Elimina archivo o directorio recursivamente |
| `find` | `-path=ruta -name=patron` | — | Busca por nombre (soporta `*` y `?`) |

### Permisos

| Comando | Parámetros obligatorios | Parámetros opcionales | Descripción |
|---|---|---|---|
| `chmod` | `-path=ruta -ugo=NNN` | `-r` (recursivo) | Cambia permisos (0–7 por dígito) |
| `chown` | `-path=ruta -usuario=X` | `-r` (recursivo) | Cambia propietario |

### EXT3

| Comando | Parámetros | Descripción |
|---|---|---|
| `journaling` | `-id=ID` | Muestra las entradas del journal de la partición |
| `loss` | `-id=ID` | Simula pérdida del sistema de archivos EXT3 |

### Reportes

| Comando | `-name=` | Salida | Descripción |
|---|---|---|---|
| `rep` | `mbr` | `.jpg` | Estructura del MBR |
| `rep` | `disk` | `.jpg` | Layout del disco con particiones |
| `rep` | `inode` | `.jpg` | Tabla de inodos |
| `rep` | `block` | `.jpg` | Bloques de datos |
| `rep` | `bm_inode` | `.txt` | Bitmap de inodos |
| `rep` | `bm_block` | `.txt` | Bitmap de bloques |
| `rep` | `sb` | `.jpg` | Superbloque |
| `rep` | `file` | `.txt` | Bloques de un archivo específico (`-path_file_ls=`) |
| `rep` | `ls` | `.jpg` | Listado de directorio con permisos (`-path_file_ls=`) |
| `rep` | `tree` | `.png` | Árbol completo del filesystem |

Parámetros del comando `rep`: `-id=ID -path=salida -name=tipo [-path_file_ls=ruta]`

---

## API REST del Backend

| Método | Endpoint | Body / Params | Respuesta |
|---|---|---|---|
| POST | `/command` | `{"command": "mkdisk ..."}` | `{"output": "OK: ..."}` |
| GET | `/status` | — | `{"status":"ok"}` |
| GET | `/disks` | — | Lista de particiones montadas |
| GET | `/browse` | `?id=491A&path=/home` | Contenido del directorio |
| GET | `/file` | `?id=491A&path=/home/f.txt` | Contenido del archivo |
| GET | `/report` | `?path=/ruta/reporte.png` | Imagen binaria del reporte |
| GET | `/journaling` | `?id=491B` | Entradas del journal EXT3 |
| GET | `/session` | — | Estado de la sesión activa |

---

## Flujo de Ejecución

```
Usuario escribe comando en UI
        │
        ▼
Frontend (api.ts) → POST /command
        │
        ▼
server.cpp — parsea argumentos con splitArgs()
        │
        ├── mkdisk → MkDisk::execute()
        ├── fdisk  → Fdisk::execute() / executeDelete() / executeAdd()
        ├── mkfs   → Mkfs::execute() → escribe EXT2 o EXT3 en disco
        ├── mkdir  → MkdirCmd::execute() → resolvePath() + allocInode()
        ├── copy   → CopyCmd::execute() → recursivo si es directorio
        ├── chmod  → ChmodCmd::execute() → actualiza i_perm en inodo
        ├── loss   → LossCmd::execute() → zeroes el área de journaling
        └── ...
        │
        ▼
ext2_writer.cpp — writeInode(), writeBlock(), writeSuperBlock()
        │
        ▼
Archivo .mia en disco
        │
        ▼ (si EXT3)
journal_manager.cpp → JournalManager::write() → graba entrada Journal
        │
        ▼
Respuesta JSON → Frontend → Terminal de salida
```

---

## Compilación

### Compilación local

```bash
cd frontend/backend
cmake -B build -S .
cd build
make -j$(nproc)
# Ejecutable: frontend/backend/build/extreamfs
```

### Ejecución local

```bash
./frontend/backend/build/extreamfs
# Escucha en http://localhost:8080
```

### Dependencias del servidor

```bash
# Ubuntu 22.04
sudo apt install -y cmake g++ graphviz
```

Graphviz es requerido para los reportes (comando `dot` internamente).
