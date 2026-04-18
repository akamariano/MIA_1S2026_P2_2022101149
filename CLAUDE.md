# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**ExtreamFS** — EXT2/EXT3 filesystem simulator for MIA (Manejo e Implementación de Archivos), USAC FIUSAC, 1S 2026. Deadline: 2026-04-23 23:59.

Two-tier system: C++ backend HTTP server simulating a virtual disk/filesystem, and a Next.js frontend for interactive terminal + file explorer.

## Build Commands

### Backend (C++)
```bash
cd frontend/backend
cmake -B build
cd build
make
# Executable: frontend/backend/build/extreamfs
```

Run the backend server:
```bash
./frontend/backend/build/extreamfs
# Listens on port 8080
```

### Frontend (Next.js)
```bash
cd frontend
npm install
npm run dev       # Dev server at localhost:3000
npm run build     # Production build
npm run lint      # ESLint
```

Environment files:
- `.env.development.local` → backend at `http://localhost:8080`
- `.env.production` → backend at `http://23.23.109.9:8080` (EC2)

## Running Tests

Tests use `.smia` script files (custom command language) loaded via the frontend UI:

1. Ensure backend is running (`curl http://localhost:8080/status`)
2. Open frontend at `http://localhost:3000`
3. Use "Cargar archivo" to upload a `.smia` file, then "Ejecutar Script"
4. Or paste individual commands in the terminal and click "Ejecutar"

**Primary test file**: `Archivo_De_Prueba_P2.smia` (359 lines) — covers all P2 features.
**Additional scripts**: `test.smia`, `test2.smia`, `test_verificacion.smia`

The test script uses paths under `/home/ubuntu/Calificacion_MIA/` — on EC2 these exist; locally you may need to create them or adjust paths.

## Architecture

```
frontend/                         # Next.js app (TypeScript, Tailwind CSS 4)
├── app/                          # Next.js App Router
├── components/                   # CommandPanel, FileExplorer, SessionBar, BlockViewer
├── services/api.ts               # HTTP client for all backend endpoints
└── backend/                      # C++ backend (lives inside frontend/ dir)
    ├── CMakeLists.txt
    └── core/
        ├── commands/             # ~42 command handlers (.cpp/.h each)
        ├── reports/              # 8 report generators (graphviz-based)
        ├── filesystem/           # EXT2/3 readers/writers, session, login/logout
        ├── disk/                 # Disk creation, partitioning (MBR/EBR)
        ├── mount/                # Mount point management
        ├── server/               # cpp-httplib HTTP server
        └── utils/                # Argument parser (splitArgs)
Documentacion/                    # Technical docs, testing guide, AWS guide
Archivo_De_Prueba_P2.smia         # Main P2 grading test script
```

### Backend API Endpoints
| Method | Endpoint | Purpose |
|--------|----------|---------|
| POST | `/command` | Execute any filesystem command |
| GET | `/status` | Health check |
| GET | `/disks` | List mounted partitions |
| GET | `/browse?id=&path=` | Navigate filesystem directory |
| GET | `/file?id=&path=` | Read file content |
| GET | `/report?path=` | Serve graphviz report images |
| GET | `/journaling?id=` | EXT3 journal entries |

### Command Flow
User types command → Frontend POST `/command` → `server.cpp` parses args → routes to handler in `core/commands/` → handler reads/writes `.mia` disk file via `ext2_reader.cpp` / `ext2_writer.cpp` → JSON response back to frontend.

### Implemented Commands
- **Disk**: `mkdisk`, `rmdisk`, `fdisk` (create/delete/add), `mount`, `unmount`, `mounted`
- **Filesystem**: `mkfs` (EXT2=`2fs`, EXT3=`3fs`)
- **Session**: `login`, `logout`
- **Users/Groups**: `mkgrp`, `rmgrp`, `mkusr`, `rmusr`, `chgrp`
- **File ops**: `mkdir`, `mkfile`, `cat`, `rename`, `copy`, `move`, `remove`, `find`
- **Permissions**: `chmod`, `chown`
- **EXT3**: `journaling`, `loss`
- **Reports**: `rep -name=` (mbr, disk, inode, block, bm_inode, bm_block, sb, file, ls, tree)

## AWS Deployment
- **Frontend**: AWS S3 bucket `extreamfs-frontend-849279003367` (static hosting)
- **Backend**: EC2 t3.micro (us-east-1), IP `23.23.109.9`, port 8080
- See `Documentacion/AWS_GUIA_RAPIDA.md` for deployment steps

## Key Implementation Notes

- Report generation uses Graphviz (`dot` command); must be installed on server
- EXT3 partition mount IDs follow pattern: `{disk_letter}{partition_number}{partition_letter}` (e.g., `491A`, `491B`)
- `.mia` files are binary virtual disk images stored at paths specified in commands
- The `-r` flag on `mkdir`/`mkfile` creates parent directories recursively
- `loss` command simulates data loss on EXT3; `journaling` shows the journal log
- `fdisk -delete=fast` marks partition deleted; `fdisk -delete=full` zeroes the data
