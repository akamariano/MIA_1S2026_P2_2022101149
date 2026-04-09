#ifndef EXT2UTILS_H
#define EXT2UTILS_H

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../core/filesystem/SuperBlock.h"
#include "../core/filesystem/Inode.h"
#include "../core/filesystem/Blocks.h"

// ============================================================
// LECTURA / ESCRITURA DE ESTRUCTURAS
// ============================================================

inline void readSuperBlock(FILE* disk, long long partStart, SuperBlock& sb) {
    fseek(disk, partStart, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, disk);
}

inline void writeSuperBlock(FILE* disk, long long partStart, const SuperBlock& sb) {
    fseek(disk, partStart, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, disk);
}

inline void readInode(FILE* disk, const SuperBlock& sb, int inodeNum, Inode& inode) {
    long long pos = sb.s_inode_start + (long long)inodeNum * sizeof(Inode);
    fseek(disk, pos, SEEK_SET);
    fread(&inode, sizeof(Inode), 1, disk);
}

inline void writeInode(FILE* disk, const SuperBlock& sb, int inodeNum, const Inode& inode) {
    long long pos = sb.s_inode_start + (long long)inodeNum * sizeof(Inode);
    fseek(disk, pos, SEEK_SET);
    fwrite(&inode, sizeof(Inode), 1, disk);
}

inline void readBlock(FILE* disk, const SuperBlock& sb, int blockNum, void* buffer) {
    long long pos = sb.s_block_start + (long long)blockNum * sizeof(FileBlock);
    fseek(disk, pos, SEEK_SET);
    fread(buffer, sizeof(FileBlock), 1, disk);
}

inline void writeBlock(FILE* disk, const SuperBlock& sb, int blockNum, const void* buffer) {
    long long pos = sb.s_block_start + (long long)blockNum * sizeof(FileBlock);
    fseek(disk, pos, SEEK_SET);
    fwrite(buffer, sizeof(FileBlock), 1, disk);
}

// ============================================================
// BITMAP
// ============================================================

inline char readBitmapInode(FILE* disk, const SuperBlock& sb, int index) {
    fseek(disk, sb.s_bm_inode_start + index, SEEK_SET);
    char val;
    fread(&val, 1, 1, disk);
    return val;
}

inline void writeBitmapInode(FILE* disk, const SuperBlock& sb, int index, char val) {
    fseek(disk, sb.s_bm_inode_start + index, SEEK_SET);
    fwrite(&val, 1, 1, disk);
}

inline char readBitmapBlock(FILE* disk, const SuperBlock& sb, int index) {
    fseek(disk, sb.s_bm_block_start + index, SEEK_SET);
    char val;
    fread(&val, 1, 1, disk);
    return val;
}

inline void writeBitmapBlock(FILE* disk, const SuperBlock& sb, int index, char val) {
    fseek(disk, sb.s_bm_block_start + index, SEEK_SET);
    fwrite(&val, 1, 1, disk);
}

// ============================================================
// ALLOCATORS
// ============================================================

// Retorna número de inodo libre, -1 si no hay
inline int allocateInode(FILE* disk, SuperBlock& sb, long long partStart) {
    for (int i = 0; i < sb.s_inodes_count; i++) {
        if (readBitmapInode(disk, sb, i) == 0) {
            writeBitmapInode(disk, sb, i, 1);
            sb.s_free_inodes_count--;
            sb.s_first_ino = i + 1; // siguiente libre estimado
            writeSuperBlock(disk, partStart, sb);
            return i;
        }
    }
    return -1;
}

// Retorna número de bloque libre, -1 si no hay
inline int allocateBlock(FILE* disk, SuperBlock& sb, long long partStart) {
    for (int i = 0; i < sb.s_blocks_count; i++) {
        if (readBitmapBlock(disk, sb, i) == 0) {
            writeBitmapBlock(disk, sb, i, 1);
            sb.s_free_blocks_count--;
            sb.s_first_blo = i + 1;
            writeSuperBlock(disk, partStart, sb);
            return i;
        }
    }
    return -1;
}

// ============================================================
// NAVEGACIÓN DE RUTAS
// ============================================================

// Divide "/home/user/docs" en ["home", "user", "docs"]
inline std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

// Busca 'name' dentro del bloque carpeta del inodo 'inodeNum'
// Retorna el número de inodo hijo, -1 si no existe
inline int findInDirectory(FILE* disk, const SuperBlock& sb,
                            int inodeNum, const std::string& name) {
    Inode inode;
    readInode(disk, sb, inodeNum, inode);
 
    // b_name solo tiene 12 bytes (11 útiles + null)
    // Truncar el nombre buscado al mismo límite para comparar correctamente
    std::string truncName = name.substr(0, 11);
 
    for (int b = 0; b < 12; b++) {
        if (inode.i_block[b] == -1) break;
        DirectoryBlock db;
        readBlock(disk, sb, inode.i_block[b], &db);
        for (int e = 0; e < 4; e++) {
            if (db.b_content[e].b_inodo == -1) continue;
            // Comparar solo hasta 11 chars para manejar nombres largos
            if (strncmp(db.b_content[e].b_name, truncName.c_str(), 11) == 0) {
                return db.b_content[e].b_inodo;
            }
        }
    }
    return -1;
}
 

// Navega la ruta completa desde root (inodo 0)
// Retorna inodo del último elemento, -1 si no existe
// Si parentInodeOut != nullptr, guarda el inodo del padre
inline int resolvePath(FILE* disk, const SuperBlock& sb,
                       const std::string& path,
                       int* parentInodeOut = nullptr) {
    std::vector<std::string> parts = splitPath(path);
    int currentInode = 0; // root siempre es inodo 0
    int parentInode  = 0;

    for (const std::string& part : parts) {
        int found = findInDirectory(disk, sb, currentInode, part);
        if (found == -1) {
            if (parentInodeOut) *parentInodeOut = currentInode;
            return -1;
        }
        parentInode  = currentInode;
        currentInode = found;
    }

    if (parentInodeOut) *parentInodeOut = parentInode;
    return currentInode;
}

// Agrega una entrada en el directorio 'parentInode'
// Retorna true si tuvo éxito
inline bool addEntryToDirectory(FILE* disk, SuperBlock& sb,
                                 long long partStart,
                                 int parentInode,
                                 const std::string& name,
                                 int childInode) {
    Inode parent;
    readInode(disk, sb, parentInode, parent);

    // Buscar espacio en bloques existentes
    for (int b = 0; b < 12; b++) {
        if (parent.i_block[b] == -1) break;

        DirectoryBlock db;
        readBlock(disk, sb, parent.i_block[b], &db);

        for (int e = 0; e < 4; e++) {
            if (db.b_content[e].b_inodo == -1) {
                strncpy(db.b_content[e].b_name, name.c_str(), 11);
                db.b_content[e].b_name[11] = '\0';
                db.b_content[e].b_inodo = childInode;
                writeBlock(disk, sb, parent.i_block[b], &db);
                return true;
            }
        }
    }

    // No hay espacio — asignar nuevo bloque al padre
    int newBlock = allocateBlock(disk, sb, partStart);
    if (newBlock == -1) return false;

    // Buscar slot libre en i_block del padre
    for (int b = 0; b < 12; b++) {
        if (parent.i_block[b] == -1) {
            DirectoryBlock db;
            memset(&db, 0, sizeof(DirectoryBlock));
            for (int e = 0; e < 4; e++) db.b_content[e].b_inodo = -1;

            strncpy(db.b_content[0].b_name, name.c_str(), 11);
            db.b_content[0].b_name[11] = '\0';
            db.b_content[0].b_inodo = childInode;

            writeBlock(disk, sb, newBlock, &db);
            parent.i_block[b] = newBlock;
            parent.i_mtime = time(nullptr);
            writeInode(disk, sb, parentInode, parent);
            return true;
        }
    }

    return false; // inodo lleno (más de 12 bloques directos)
}

#endif