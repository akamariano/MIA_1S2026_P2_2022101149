#include "copy_cmd.h"
#include "../filesystem/session_manager.h"
#include "../filesystem/permissions.h"
#include "../filesystem/journal_manager.h"
#include "../filesystem/EXT2Utils.h"
#include "../filesystem/ext2_writer.h"
#include "../mount/mount_manager.h"
#include "../core/filesystem/SuperBlock.h"
#include "../core/filesystem/Inode.h"
#include "../core/filesystem/Blocks.h"
#include <iostream>
#include <cstring>
#include <string>
#include <ctime>
using namespace std;

// Copia recursiva de un inodo hacia destParentInode con nombre 'name'
// No usa mkfile para evitar que resuelva paths desde root
static void copyRecursive(FILE* disk, SuperBlock& sb, long long partStart,
                          int srcInode, int destParentInode,
                          const string& name,
                          int uid, int gid) {
    Inode src;
    readInode(disk, sb, srcInode, src);

    // Verificar permiso de lectura en origen
    if (!Permissions::canRead(src)) {
        cout << "ADVERTENCIA: Sin permiso de lectura en '" << name << "', omitiendo\n";
        return;
    }

    if (src.i_type == '0') {
        // ── CARPETA: crear en destino y copiar hijos ──
        int newDir = EXT2Writer::createDirPublic(disk, sb, partStart,
                                                  destParentInode, name,
                                                  uid, gid);
        if (newDir == -1) return;

        for (int b = 0; b < 12; b++) {
            if (src.i_block[b] == -1) break;
            DirectoryBlock db;
            memset(&db, 0, sizeof(DirectoryBlock));
            readBlock(disk, sb, src.i_block[b], &db);
            for (int e = 0; e < 4; e++) {
                if (db.b_content[e].b_inodo == -1) continue;
                string ename(db.b_content[e].b_name,
                             strnlen(db.b_content[e].b_name, 12));
                if (ename == "." || ename == "..") continue;
                copyRecursive(disk, sb, partStart,
                              db.b_content[e].b_inodo,
                              newDir, ename, uid, gid);
            }
        }

    } else {
        // ── ARCHIVO: leer contenido del origen ──
        string content;
        for (int b = 0; b < 12; b++) {
            if (src.i_block[b] == -1) break;
            FileBlock fb;
            memset(&fb, 0, sizeof(FileBlock));
            readBlock(disk, sb, src.i_block[b], &fb);
            int toRead = min((int)sizeof(fb.b_content),
                             src.i_size - (int)content.size());
            if (toRead <= 0) break;
            content.append(fb.b_content, toRead);
        }
        // Bloque indirecto simple
        if (src.i_block[12] != -1) {
            PointerBlock pb;
            readBlock(disk, sb, src.i_block[12], &pb);
            for (int i = 0; i < 16; i++) {
                if (pb.b_pointers[i] == -1) break;
                FileBlock fb;
                memset(&fb, 0, sizeof(FileBlock));
                readBlock(disk, sb, pb.b_pointers[i], &fb);
                int toRead = min((int)sizeof(fb.b_content),
                                 src.i_size - (int)content.size());
                if (toRead <= 0) break;
                content.append(fb.b_content, toRead);
            }
        }

        // ── Crear nuevo inodo de archivo directamente en destParentInode ──
        // NO usar mkfile() porque resuelve paths desde root
        int newInodeNum = allocateInode(disk, sb, partStart);
        if (newInodeNum == -1) {
            cout << "ERROR: No hay inodos libres al copiar '" << name << "'\n";
            return;
        }

        Inode newInode;
        memset(&newInode, 0, sizeof(Inode));
        newInode.i_uid   = uid;
        newInode.i_gid   = gid;
        newInode.i_size  = 0;
        newInode.i_type  = '1'; // archivo
        newInode.i_perm  = src.i_perm; // conservar permisos del original
        newInode.i_atime = newInode.i_ctime = newInode.i_mtime = time(nullptr);
        for (int i = 0; i < 15; i++) newInode.i_block[i] = -1;

        // Escribir inodo vacío primero
        writeInode(disk, sb, newInodeNum, newInode);

        // Escribir contenido si hay
        if (!content.empty()) {
            EXT2Writer::writeFileContentPublic(disk, sb, partStart,
                                               newInode, newInodeNum, content);
        } else {
            writeInode(disk, sb, newInodeNum, newInode);
        }

        // Registrar en el directorio destino
        if (!addEntryToDirectory(disk, sb, partStart,
                                  destParentInode, name, newInodeNum)) {
            cout << "ERROR: No se pudo registrar '" << name << "' en destino\n";
            return;
        }

        cout << "OK: Archivo '" << name << "' creado correctamente\n";
    }
}

void CopyCmd::execute(const string& path, const string& destino) {

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

    // Verificar origen
    int srcInode = resolvePath(disk, sb, path);
    if (srcInode == -1) {
        cout << "ERROR: La ruta origen '" << path << "' no existe\n";
        fclose(disk); return;
    }

    // Verificar destino existe y es carpeta
    int destInode = resolvePath(disk, sb, destino);
    if (destInode == -1) {
        cout << "ERROR: La carpeta destino '" << destino << "' no existe\n";
        fclose(disk); return;
    }

    Inode destInodeData;
    readInode(disk, sb, destInode, destInodeData);
    if (destInodeData.i_type != '0') {
        cout << "ERROR: El destino '" << destino << "' no es una carpeta\n";
        fclose(disk); return;
    }
    if (!Permissions::canWrite(destInodeData)) {
        cout << "ERROR: No tiene permiso de escritura en '" << destino << "'\n";
        fclose(disk); return;
    }

    // Obtener nombre del origen
    string srcName = path;
    size_t lastSlash = path.rfind('/');
    if (lastSlash != string::npos)
        srcName = path.substr(lastSlash + 1);

    int uid = SessionManager::get().uid;
    int gid = SessionManager::get().gid;

    // Copiar recursivamente con destino correcto
    copyRecursive(disk, sb, part->start,
                  srcInode, destInode, srcName, uid, gid);

    // Journaling
    JournalManager::write(disk, sb, part->start, "copy", path, destino);

    writeSuperBlock(disk, part->start, sb);
    fclose(disk);

    cout << "OK: '" << path << "' copiado a '" << destino << "'\n";
}