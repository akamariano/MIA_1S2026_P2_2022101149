#ifndef JOURNAL_MANAGER_H
#define JOURNAL_MANAGER_H

#include "SuperBlock.h"
#include "Journal.h"
#include "EXT2Utils.h"
#include <string>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <vector>
#include <algorithm>

class JournalManager {
public:

    // Escribe una entrada en el journal (solo si es EXT3)
    static void write(FILE* disk, SuperBlock& sb, long long partStart,
                      const std::string& operation,
                      const std::string& path,
                      const std::string& content = "") {

        if (sb.s_filesystem_type != 3) return;

        int numJournals = sb.s_inodes_count;
        long long journalStart = partStart + sizeof(SuperBlock);

        // Buscar slot libre (j_count == 0) o el más antiguo (circular)
        int targetSlot = -1;
        int minCount   = -1;

        for (int i = 0; i < numJournals; i++) {
            Journal j;
            long long pos = journalStart + (long long)i * sizeof(Journal);
            fseek(disk, pos, SEEK_SET);
            fread(&j, sizeof(Journal), 1, disk);

            if (j.j_count == 0) {
                targetSlot = i;
                break;
            }
            if (minCount == -1 || j.j_count < minCount) {
                minCount   = j.j_count;
                targetSlot = i;
            }
        }

        if (targetSlot == -1) return;

        // Obtener count máximo actual
        int maxCount = 0;
        for (int i = 0; i < numJournals; i++) {
            Journal j;
            long long pos = journalStart + (long long)i * sizeof(Journal);
            fseek(disk, pos, SEEK_SET);
            fread(&j, sizeof(Journal), 1, disk);
            if (j.j_count > maxCount) maxCount = j.j_count;
        }

        // Crear nueva entrada
        Journal entry;
        memset(&entry, 0, sizeof(Journal));
        entry.j_count = maxCount + 1;

        // Operacion: hasta 9 chars + null
        strncpy(entry.j_content.i_operation, operation.c_str(),
                sizeof(entry.j_content.i_operation) - 1);

        // Path: si es más largo que 31 chars, guardar los últimos 31
        // para preservar la parte más informativa (nombre de archivo)
        const int PATH_MAX_LEN = sizeof(entry.j_content.i_path) - 1;
        std::string pathToStore = path;
        if ((int)pathToStore.size() > PATH_MAX_LEN) {
            // Intentar recortar desde un '/' para mantener legibilidad
            std::string trimmed = pathToStore.substr(pathToStore.size() - PATH_MAX_LEN);
            size_t slash = trimmed.find('/');
            if (slash != std::string::npos)
                pathToStore = trimmed.substr(slash); // empieza desde /
            else
                pathToStore = trimmed;
        }
        strncpy(entry.j_content.i_path, pathToStore.c_str(), PATH_MAX_LEN);

        // Content: hasta 63 chars + null
        const int CONTENT_MAX_LEN = sizeof(entry.j_content.i_content) - 1;
        std::string contentToStore = content;
        if ((int)contentToStore.size() > CONTENT_MAX_LEN)
            contentToStore = contentToStore.substr(0, CONTENT_MAX_LEN);
        strncpy(entry.j_content.i_content, contentToStore.c_str(), CONTENT_MAX_LEN);

        entry.j_content.i_date = (float)time(nullptr);

        // Escribir en disco
        long long pos = journalStart + (long long)targetSlot * sizeof(Journal);
        fseek(disk, pos, SEEK_SET);
        fwrite(&entry, sizeof(Journal), 1, disk);
    }

    // Leer todos los journals activos de una partición
    static std::vector<Journal> readAll(FILE* disk, SuperBlock& sb,
                                         long long partStart) {
        std::vector<Journal> result;
        if (sb.s_filesystem_type != 3) return result;

        int numJournals = sb.s_inodes_count;
        long long journalStart = partStart + sizeof(SuperBlock);

        for (int i = 0; i < numJournals; i++) {
            Journal j;
            long long pos = journalStart + (long long)i * sizeof(Journal);
            fseek(disk, pos, SEEK_SET);
            fread(&j, sizeof(Journal), 1, disk);
            if (j.j_count > 0) result.push_back(j);
        }

        // Ordenar por j_count ascendente
        std::sort(result.begin(), result.end(),
                  [](const Journal& a, const Journal& b){
                      return a.j_count < b.j_count;
                  });
        return result;
    }
};

#endif