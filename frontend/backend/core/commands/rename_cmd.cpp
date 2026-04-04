#include "rename_cmd.h"
#include "../filesystem/session_manager.h"
#include "../filesystem/permissions.h"
#include "../filesystem/journal_manager.h"
#include "../filesystem/EXT2Utils.h"
#include "../mount/mount_manager.h"
#include "../core/filesystem/SuperBlock.h"
#include "../core/filesystem/Inode.h"
#include "../core/filesystem/Blocks.h"
#include <iostream>
#include <cstring>
using namespace std;

void RenameCmd::execute(const string& path, const string& newName) {

    if (!SessionManager::isActive()) {
        cout << "ERROR: No hay sesión activa\n"; return;
    }

    MountedPartition* part = MountManager::getMountedById(
                                SessionManager::get().partitionId);
    if (!part) { cout << "ERROR: Partición no encontrada\n"; return; }

    FILE* disk = fopen(part->path.c_str(), "rb+");
    if (!disk) { cout << "ERROR: No se pudo abrir el disco\n"; return; }

    SuperBlock sb;
    readSuperBlock(disk, part->start, sb);

    // Resolver path y obtener el inodo padre en una sola llamada
    int parentInode = -1;
    int targetInode = resolvePath(disk, sb, path, &parentInode);

    if (targetInode == -1) {
        cout << "ERROR: La ruta '" << path << "' no existe\n";
        fclose(disk); return;
    }

    // Verificar permisos de escritura sobre el archivo/carpeta
    Inode targetInodeData;
    readInode(disk, sb, targetInode, targetInodeData);
    if (!Permissions::canWrite(targetInodeData)) {
        cout << "ERROR: No tiene permiso de escritura sobre '" << path << "'\n";
        fclose(disk); return;
    }

    // Verificar que el nuevo nombre no existe ya en el padre
    int existing = findInDirectory(disk, sb, parentInode, newName);
    if (existing != -1) {
        cout << "ERROR: Ya existe '" << newName << "' en el directorio\n";
        fclose(disk); return;
    }

    // Buscar la entrada en el DirectoryBlock del padre y renombrarla
    Inode parent;
    readInode(disk, sb, parentInode, parent);
    bool renamed = false;

    for (int b = 0; b < 12 && !renamed; b++) {
        if (parent.i_block[b] == -1) break;

        DirectoryBlock db;
        readBlock(disk, sb, parent.i_block[b], &db);

        for (int e = 0; e < 4; e++) {
            if (db.b_content[e].b_inodo == targetInode) {
                // Limpiar el nombre anterior y escribir el nuevo
                memset(db.b_content[e].b_name, 0, 12);
                strncpy(db.b_content[e].b_name, newName.c_str(), 11);
                db.b_content[e].b_name[11] = '\0';
                writeBlock(disk, sb, parent.i_block[b], &db);
                renamed = true;
                break;
            }
        }
    }

    if (!renamed) {
        cout << "ERROR: No se pudo renombrar (entrada no encontrada en directorio padre)\n";
        fclose(disk); return;
    }

    // Actualizar timestamps del padre e inodo renombrado
    time_t now = time(nullptr);
    parent.i_mtime = now;
    writeInode(disk, sb, parentInode, parent);

    targetInodeData.i_ctime = now;
    writeInode(disk, sb, targetInode, targetInodeData);

    // Journaling (solo EXT3)
    JournalManager::write(disk, sb, part->start, "rename", path, newName);

    writeSuperBlock(disk, part->start, sb);
    fclose(disk);

    cout << "OK: '" << path << "' renombrado a '" << newName << "'\n";
}