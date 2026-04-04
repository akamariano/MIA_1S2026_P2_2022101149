#ifndef EXT2_WRITER_H
#define EXT2_WRITER_H

#include <string>
#include <cstdio>
#include "../core/filesystem/SuperBlock.h"
#include "../core/filesystem/Inode.h"
#include "../core/filesystem/Blocks.h"

class EXT2Writer {
public:
    // Crea carpeta en la partición
    static bool mkdir(FILE* disk, SuperBlock& sb, long long partStart,
                      const std::string& path, bool createParents,
                      int uid, int gid);

    // Crea archivo en la partición
    static bool mkfile(FILE* disk, SuperBlock& sb, long long partStart,
                       const std::string& path, bool createParents,
                       int size, const std::string& content,
                       int uid, int gid);

    // Escribe users.txt inicial (llamado desde mkfs)
    static bool createUsersFile(FILE* disk, SuperBlock& sb,
                                long long partStart);

    // Reescribe users.txt completo con nuevo contenido
    static bool writeUsersFile(FILE* disk, SuperBlock& sb,
                               long long partStart,
                               const std::string& content);

    // Lee users.txt completo como string
    static std::string readUsersFile(FILE* disk, SuperBlock& sb,
                                     long long partStart);

    // Crea directorio público (usado por copy)
    static int createDirPublic(FILE* disk, SuperBlock& sb,
                                long long partStart,
                                int parentInode,
                                const std::string& name,
                                int uid, int gid);

    // Escribe contenido en bloques de archivo (público para copy)
    static bool writeFileContentPublic(FILE* disk, SuperBlock& sb,
                                       long long partStart,
                                       Inode& fileInode, int inodeNum,
                                       const std::string& content);

private:
    // Crea un inodo de carpeta y lo registra en su padre
    static int createDirectory(FILE* disk, SuperBlock& sb,
                               long long partStart,
                               int parentInode,
                               const std::string& name,
                               int uid, int gid);

    // Escribe contenido en bloques de archivo (privado, llamado internamente)
    static bool writeFileContent(FILE* disk, SuperBlock& sb,
                                 long long partStart,
                                 Inode& fileInode, int inodeNum,
                                 const std::string& content);
};

#endif