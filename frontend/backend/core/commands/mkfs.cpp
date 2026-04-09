#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstring>
#include "mkfs.h"
#include "../mount/mount_manager.h"
#include "../core/filesystem/SuperBlock.h"
#include "../core/filesystem/Inode.h"
#include "../core/filesystem/Blocks.h"
#include "../core/filesystem/Journal.h"
#include "../core/filesystem/ext2_writer.h"
using namespace std;

void Mkfs::execute(string id, string type) {

    MountedPartition* part = MountManager::getMountedById(id);
    if (!part) {
        cout << "ERROR: ID no montado\n";
        return;
    }

    FILE* file = fopen(part->path.c_str(), "rb+");
    if (!file) {
        cout << "ERROR: No se pudo abrir el disco\n";
        return;
    }

    long long partitionStart = part->start;
    long long partitionSize  = part->size;

    bool isExt3 = (type == "ext3" || type == "3fs");

    int n = 0;

    if (isExt3) {
        // Fórmula EXT3:
        // tamaño = SB + n*sizeof(Journal) + n + 3n + n*sizeof(Inode) + 3n*sizeof(FileBlock)
        double denom = (double)sizeof(Journal)
                     + 1.0 + 3.0
                     + (double)sizeof(Inode)
                     + 3.0 * (double)sizeof(FileBlock);
        n = (int)floor((partitionSize - (double)sizeof(SuperBlock)) / denom);
    } else {
        // Fórmula EXT2:
        // tamaño = SB + n + 3n + n*sizeof(Inode) + 3n*sizeof(FileBlock)
        double denom = 1.0 + 3.0
                     + (double)sizeof(Inode)
                     + 3.0 * (double)sizeof(FileBlock);
        n = (int)floor((partitionSize - (double)sizeof(SuperBlock)) / denom);
    }

    if (n <= 0) {
        cout << "ERROR: Partición demasiado pequeña\n";
        fclose(file);
        return;
    }

    int numInodes  = n;
    int numBlocks  = 3 * n;
    int numJournal = isExt3 ? n : 0;

    // Calcular posiciones
    long long journal_start  = partitionStart + sizeof(SuperBlock);
    long long bm_inode_start = isExt3
                             ? journal_start + (long long)numJournal * sizeof(Journal)
                             : journal_start;
    long long bm_block_start = bm_inode_start + numInodes;
    long long inode_start    = bm_block_start + numBlocks;
    long long block_start    = inode_start + (long long)numInodes * sizeof(Inode);

    // Crear SuperBloque
    SuperBlock sb;
    memset(&sb, 0, sizeof(SuperBlock));
    sb.s_filesystem_type   = isExt3 ? 3 : 2;
    sb.s_inodes_count      = numInodes;
    sb.s_blocks_count      = numBlocks;
    sb.s_free_inodes_count = numInodes - 1;
    sb.s_free_blocks_count = numBlocks - 1;
    sb.s_mtime             = time(nullptr);
    sb.s_umtime            = 0;
    sb.s_mnt_count         = 1;
    sb.s_magic             = 0xEF53;
    sb.s_inode_size        = sizeof(Inode);
    sb.s_block_size        = sizeof(FileBlock);
    sb.s_first_ino         = 1;
    sb.s_first_blo         = 1;
    sb.s_bm_inode_start    = bm_inode_start;
    sb.s_bm_block_start    = bm_block_start;
    sb.s_inode_start       = inode_start;
    sb.s_block_start       = block_start;

    // Inicializar bitmaps
    vector<unsigned char> bm_inode(numInodes, 0);
    vector<unsigned char> bm_block(numBlocks, 0);
    bm_inode[0] = 1;
    bm_block[0] = 1;

    // Crear inode raíz
    Inode root;
    memset(&root, 0, sizeof(Inode));
    root.i_uid   = 1;
    root.i_gid   = 1;
    root.i_size  = sizeof(DirectoryBlock);
    root.i_type  = '0';
    root.i_perm  = 664;
    root.i_atime = root.i_ctime = root.i_mtime = time(nullptr);
    for (int i = 0; i < 15; i++) root.i_block[i] = -1;
    root.i_block[0] = 0;

    // Crear bloque raíz
    DirectoryBlock rootBlock;
    memset(&rootBlock, 0, sizeof(DirectoryBlock));
    strncpy(rootBlock.b_content[0].b_name, ".",  11);
    rootBlock.b_content[0].b_inodo = 0;
    strncpy(rootBlock.b_content[1].b_name, "..", 11);
    rootBlock.b_content[1].b_inodo = 0;
    rootBlock.b_content[2].b_inodo = -1;
    rootBlock.b_content[3].b_inodo = -1;

    // Escribir SuperBloque
    fseek(file, partitionStart, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, file);

    // Inicializar journals en 0 si es EXT3
    if (isExt3) {
        fseek(file, journal_start, SEEK_SET);
        Journal emptyJournal;
        memset(&emptyJournal, 0, sizeof(Journal));
        for (int i = 0; i < numJournal; i++) {
            fwrite(&emptyJournal, sizeof(Journal), 1, file);
        }
    }

    // Escribir bitmaps
    fseek(file, bm_inode_start, SEEK_SET);
    fwrite(bm_inode.data(), 1, numInodes, file);

    fseek(file, bm_block_start, SEEK_SET);
    fwrite(bm_block.data(), 1, numBlocks, file);

    // Escribir inode raíz
    fseek(file, inode_start, SEEK_SET);
    fwrite(&root, sizeof(Inode), 1, file);

    // Escribir bloque raíz
    fseek(file, block_start, SEEK_SET);
    fwrite(&rootBlock, sizeof(DirectoryBlock), 1, file);

    // Crear users.txt
    EXT2Writer::createUsersFile(file, sb, partitionStart);

    fclose(file);

    cout << "OK: MKFS realizado correctamente (" << (isExt3 ? "EXT3" : "EXT2") << ")\n";
    cout << "    Inodos  : " << numInodes << "\n";
    cout << "    Bloques : " << numBlocks << "\n";
    if (isExt3)
        cout << "    Journals: " << numJournal << "\n";
    cout << "    Inicio superblock : " << partitionStart  << "\n";
    if (isExt3)
        cout << "    Inicio journal    : " << journal_start   << "\n";
    cout << "    Inicio bm_inodos  : " << bm_inode_start  << "\n";
    cout << "    Inicio bm_bloques : " << bm_block_start  << "\n";
    cout << "    Inicio inodos     : " << inode_start     << "\n";
    cout << "    Inicio bloques    : " << block_start     << "\n";
}