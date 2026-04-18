#include "chown_cmd.h"
#include "../filesystem/session_manager.h"
#include "../filesystem/permissions.h"
#include "../filesystem/journal_manager.h"
#include "../filesystem/users_manager.h"
#include "../filesystem/EXT2Utils.h"
#include "../filesystem/ext2_writer.h"
#include "../mount/mount_manager.h"
#include "../core/filesystem/SuperBlock.h"
#include "../core/filesystem/Inode.h"
#include "../core/filesystem/Blocks.h"
#include <iostream>
#include <cstring>
using namespace std;

static void chownRecursive(FILE* disk, SuperBlock& sb,
                            int inodeNum, int newUid, int newGid,
                            int currentUid, bool isRoot) {
    Inode inode;
    readInode(disk, sb, inodeNum, inode);

    // Solo root o propietario puede cambiar
    if (isRoot || inode.i_uid == currentUid) {
        inode.i_uid = newUid;
        inode.i_gid = newGid;
        writeInode(disk, sb, inodeNum, inode);
    }

    if (inode.i_type != '0') return;

    // Recursivo en carpetas
    for (int b = 0; b < 12; b++) {
        if (inode.i_block[b] == -1) break;
        DirectoryBlock db;
        memset(&db, 0, sizeof(DirectoryBlock));
        readBlock(disk, sb, inode.i_block[b], &db);
        for (int e = 0; e < 4; e++) {
            if (db.b_content[e].b_inodo == -1) continue;
            string ename(db.b_content[e].b_name,
                         strnlen(db.b_content[e].b_name, 12));
            if (ename == "." || ename == "..") continue;
            chownRecursive(disk, sb, db.b_content[e].b_inodo,
                           newUid, newGid, currentUid, isRoot);
        }
    }
}

void ChownCmd::execute(const string& path,
                        const string& usuario,
                        bool recursive) {

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

    // Verificar que el path existe
    int targetInode = resolvePath(disk, sb, path);
    if (targetInode == -1) {
        cout << "ERROR: La ruta '" << path << "' no existe\n";
        fclose(disk); return;
    }

    // Buscar uid y gid del nuevo usuario en users.txt
    string usersContent = EXT2Writer::readUsersFile(disk, sb, part->start);
    UsersData data = parseUsers(usersContent);

    int newUid = -1, newGid = -1;
    string newGroup = "";
    for (auto& u : data.users) {
        if (u.username == usuario && u.active) {
            newUid  = u.uid;
            newGroup = u.group;
            break;
        }
    }

    // Fallback: search other mounted partitions if user not found locally
    if (newUid == -1) {
        for (const auto& mp : MountManager::getAll()) {
            if (mp.path == part->path && mp.start == part->start) continue;
            FILE* otherDisk = fopen(mp.path.c_str(), "rb");
            if (!otherDisk) continue;
            SuperBlock otherSb;
            readSuperBlock(otherDisk, mp.start, otherSb);
            string otherUsers = EXT2Writer::readUsersFile(otherDisk, otherSb, mp.start);
            fclose(otherDisk);
            UsersData otherData = parseUsers(otherUsers);
            for (auto& u : otherData.users) {
                if (u.username == usuario && u.active) {
                    newUid  = u.uid;
                    newGroup = u.group;
                    for (auto& g : otherData.groups) {
                        if (g.name == newGroup && g.active) { newGid = g.gid; break; }
                    }
                    break;
                }
            }
            if (newUid != -1) break;
        }
    }

    if (newUid == -1) {
        cout << "ERROR: El usuario '" << usuario << "' no existe\n";
        fclose(disk); return;
    }

    for (auto& g : data.groups) {
        if (g.name == newGroup && g.active) {
            newGid = g.gid; break;
        }
    }

    bool isRoot   = SessionManager::isRoot();
    int currentUid = SessionManager::get().uid;

    // Verificar permisos — solo root o propietario
    Inode targetInodeData;
    readInode(disk, sb, targetInode, targetInodeData);
    if (!isRoot && targetInodeData.i_uid != currentUid) {
        cout << "ERROR: Solo puede cambiar propietario de sus propios archivos\n";
        fclose(disk); return;
    }

    if (recursive) {
        chownRecursive(disk, sb, targetInode, newUid, newGid,
                       currentUid, isRoot);
    } else {
        targetInodeData.i_uid = newUid;
        targetInodeData.i_gid = newGid;
        writeInode(disk, sb, targetInode, targetInodeData);
    }

    // Journaling
    JournalManager::write(disk, sb, part->start, "chown", path, usuario);

    writeSuperBlock(disk, part->start, sb);
    fclose(disk);

    cout << "OK: Propietario de '" << path << "' cambiado a '" << usuario << "'"
         << (recursive ? " (recursivo)" : "") << "\n";
}