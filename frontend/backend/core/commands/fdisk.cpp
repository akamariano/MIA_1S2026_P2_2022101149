#include "fdisk.h"
#include "../mount/mount_manager.h"
#include <iostream>
#include <cstdio>
#include <cstring>
#include "../disk/mbr.h"
#include "../disk/ebr.h"
#include <vector>
#include <algorithm>
#include <climits>
using namespace std;

struct EspacioLibre {
    long long inicio;
    long long tamano;
};

bool FDisk::existeNombreLogica(FILE* disk, long long startExtendida, string nombre) {
    EBR ebr;
    long long pos = startExtendida;
    while (pos != -1) {
        fseek(disk, pos, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, disk);
        if (ebr.part_size > 0)
            if (strcmp(ebr.part_name, nombre.c_str()) == 0)
                return true;
        pos = ebr.part_next;
    }
    return false;
}

void FDisk::printLogicas(FILE* disk, long long startExtendida) {
    EBR ebr;
    long long pos = startExtendida;
    int contador = 0;
    cout << "\n----- PARTICIONES LOGICAS -----\n";
    while (pos != -1) {
        fseek(disk, pos, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, disk);
        if (ebr.part_size > 0) {
            cout << "EBR #" << contador << "\n";
            cout << "  Name  : " << ebr.part_name  << "\n";
            cout << "  Start : " << ebr.part_start << "\n";
            cout << "  Size  : " << ebr.part_size  << "\n";
            cout << "  Next  : " << ebr.part_next  << "\n";
            cout << "---------------------------\n";
            contador++;
        }
        pos = ebr.part_next;
    }
    if (contador == 0) cout << "No hay particiones logicas creadas.\n";
}

// ============================================================
// EXECUTE — crear partición (original)
// ============================================================
void FDisk::execute(int size, char unit, string path,
                    char type, string fit, string name) {

    if (size <= 0) { cout << "ERROR: El tamaño debe ser mayor a 0\n"; return; }

    if (fit.empty()) fit = "WF";
    if (fit != "BF" && fit != "FF" && fit != "WF") {
        cout << "ERROR: Fit inválido (use BF, FF o WF)\n"; return;
    }

    long long bytes = size;
    if      (unit == 'K' || unit == 'k') bytes *= 1024;
    else if (unit == 'M' || unit == 'm') bytes *= 1024 * 1024;
    else if (unit != 'B' && unit != 'b') { cout << "ERROR: Unidad inválida\n"; return; }

    FILE* file = fopen(path.c_str(), "rb+");
    if (!file) { cout << "ERROR: No se pudo abrir el disco\n"; return; }

    MBR mbr;
    fseek(file, 0, SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, file);

    if (type != 'P' && type != 'E' && type != 'L') {
        cout << "ERROR: Tipo de partición inválido\n"; fclose(file); return;
    }

    int primaryExtendedCount = 0;
    bool extendedExists = false;
    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '1') {
            if (mbr.mbr_partitions[i].part_type == 'P' ||
                mbr.mbr_partitions[i].part_type == 'E') {
                primaryExtendedCount++;
                if (mbr.mbr_partitions[i].part_type == 'E') extendedExists = true;
            }
        }
    }

    if (type == 'E' && extendedExists) {
        cout << "ERROR: Ya existe una partición extendida\n"; fclose(file); return;
    }
    if ((type == 'P' || type == 'E') && primaryExtendedCount >= 4) {
        cout << "ERROR: Límite de 4 particiones alcanzado\n"; fclose(file); return;
    }
    if (type == 'L' && !extendedExists) {
        cout << "ERROR: No existe partición extendida\n"; fclose(file); return;
    }

    // ── PARTICIÓN LÓGICA ──
    if (type == 'L') {
        Partition extendedPartition;
        bool found = false;
        for (int i = 0; i < 4; i++) {
            if (mbr.mbr_partitions[i].part_status == '1' &&
                mbr.mbr_partitions[i].part_type == 'E') {
                extendedPartition = mbr.mbr_partitions[i];
                found = true; break;
            }
        }
        if (!found) { cout << "ERROR: No se encontró la partición extendida\n"; fclose(file); return; }

        long long inicio_ext = extendedPartition.part_start;
        long long fin_ext    = extendedPartition.part_start + extendedPartition.part_size;

        if (existeNombreLogica(file, inicio_ext, name)) {
            cout << "ERROR: Ya existe una partición lógica con ese nombre\n"; fclose(file); return;
        }

        long long ebrPosition = inicio_ext;
        EBR ebr;
        fseek(file, ebrPosition, SEEK_SET);
        fread(&ebr, sizeof(EBR), 1, file);

        if (ebr.part_start == -1) {
            long long logicStart = ebrPosition + sizeof(EBR);
            if (logicStart + bytes > fin_ext) {
                cout << "ERROR: No hay espacio dentro de la extendida\n"; fclose(file); return;
            }
            ebr.part_mount = '0';
            strncpy(ebr.part_fit, fit.c_str(), 2); ebr.part_fit[2] = '\0';
            ebr.part_start = logicStart; ebr.part_size = bytes; ebr.part_next = -1;
            memset(ebr.part_name, 0, 16); strncpy(ebr.part_name, name.c_str(), 15);
            fseek(file, ebrPosition, SEEK_SET);
            fwrite(&ebr, sizeof(EBR), 1, file);
            printLogicas(file, inicio_ext);
            fclose(file);
            cout << "OK: Partición lógica creada correctamente\n"; return;
        }

        long long currentEBRPos = inicio_ext;
        EBR currentEBR;
        while (true) {
            fseek(file, currentEBRPos, SEEK_SET);
            fread(&currentEBR, sizeof(EBR), 1, file);
            if (currentEBR.part_next == -1) break;
            currentEBRPos = currentEBR.part_next;
        }

        long long newEBRPos     = currentEBR.part_start + currentEBR.part_size;
        long long newLogicStart = newEBRPos + sizeof(EBR);
        if (newLogicStart + bytes > fin_ext) {
            cout << "ERROR: No hay espacio suficiente dentro de la extendida\n"; fclose(file); return;
        }
        currentEBR.part_next = newEBRPos;
        fseek(file, currentEBRPos, SEEK_SET);
        fwrite(&currentEBR, sizeof(EBR), 1, file);

        EBR newEBR;
        memset(&newEBR, 0, sizeof(EBR));
        newEBR.part_mount = '0';
        strncpy(newEBR.part_fit, fit.c_str(), 2); newEBR.part_fit[2] = '\0';
        newEBR.part_start = newLogicStart; newEBR.part_size = bytes; newEBR.part_next = -1;
        memset(newEBR.part_name, 0, 16); strncpy(newEBR.part_name, name.c_str(), 15);
        fseek(file, newEBRPos, SEEK_SET);
        fwrite(&newEBR, sizeof(EBR), 1, file);
        printLogicas(file, inicio_ext);
        fclose(file);
        cout << "OK: Partición lógica creada correctamente\n"; return;
    }

    // ── VALIDAR NOMBRE DUPLICADO ──
    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '1')
            if (strncmp(mbr.mbr_partitions[i].part_name, name.c_str(), 16) == 0) {
                cout << "ERROR: Ya existe una partición con ese nombre\n"; fclose(file); return;
            }
    }

    // ── SLOT LIBRE ──
    int index = -1;
    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '0') { index = i; break; }
    }
    if (index == -1) { cout << "ERROR: No hay espacio en la tabla de particiones\n"; fclose(file); return; }

    // ── ESPACIOS LIBRES ──
    vector<EspacioLibre> libres;
    long long diskStart = sizeof(MBR);
    long long diskEnd   = mbr.mbr_tamano;
    vector<Partition> activas;
    for (int i = 0; i < 4; i++)
        if (mbr.mbr_partitions[i].part_status == '1') activas.push_back(mbr.mbr_partitions[i]);
    sort(activas.begin(), activas.end(), [](Partition a, Partition b){ return a.part_start < b.part_start; });
    long long current = diskStart;
    for (auto& p : activas) {
        if (p.part_start > current) libres.push_back({current, p.part_start - current});
        current = p.part_start + p.part_size;
    }
    if (current < diskEnd) libres.push_back({current, diskEnd - current});

    // ── FIT ──
    long long start = -1;
    if (fit == "FF") {
        for (auto& e : libres) { if (e.tamano >= bytes) { start = e.inicio; break; } }
    } else if (fit == "BF") {
        long long mejor = LLONG_MAX;
        for (auto& e : libres) if (e.tamano >= bytes && e.tamano < mejor) { mejor = e.tamano; start = e.inicio; }
    } else if (fit == "WF") {
        long long peor = -1;
        for (auto& e : libres) if (e.tamano >= bytes && e.tamano > peor) { peor = e.tamano; start = e.inicio; }
    }
    if (start == -1) { cout << "ERROR: No hay espacio suficiente en el disco\n"; fclose(file); return; }

    // ── CREAR ──
    Partition newPartition;
    memset(&newPartition, 0, sizeof(Partition));
    newPartition.part_status = '1';
    newPartition.part_type   = type;
    strncpy(newPartition.part_fit, fit.c_str(), 2); newPartition.part_fit[2] = '\0';
    newPartition.part_start  = start;
    newPartition.part_size   = bytes;
    memset(newPartition.part_name, 0, 16);
    strncpy(newPartition.part_name, name.c_str(), 15);
    mbr.mbr_partitions[index] = newPartition;
    fseek(file, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MBR), 1, file);

    if (type == 'E') {
        EBR firstEBR;
        memset(&firstEBR, 0, sizeof(EBR));
        firstEBR.part_mount = '0'; strncpy(firstEBR.part_fit, "0", 1);
        firstEBR.part_start = -1; firstEBR.part_size = 0; firstEBR.part_next = -1;
        memset(firstEBR.part_name, 0, 16);
        fseek(file, start, SEEK_SET);
        fwrite(&firstEBR, sizeof(EBR), 1, file);
    }

    fclose(file);
    cout << "OK: Partición '" << name << "' creada correctamente\n";
}

// ============================================================
// EXECUTE DELETE
// ============================================================
void FDisk::executeDelete(const string& deleteType, const string& name,
                           const string& path) {

    if (deleteType != "fast" && deleteType != "full") {
        cout << "ERROR: -delete debe ser 'fast' o 'full'\n"; return;
    }
    if (name.empty() || path.empty()) {
        cout << "ERROR: -name y -path son obligatorios para -delete\n"; return;
    }

    // Verificar que no esté montada
    for (auto& m : MountManager::getAll()) {
        if (m.path == path && m.name == name) {
            cout << "ERROR: La partición '" << name
                 << "' está montada. Desmóntela antes de eliminarla\n";
            return;
        }
    }

    FILE* file = fopen(path.c_str(), "rb+");
    if (!file) { cout << "ERROR: No se pudo abrir el disco\n"; return; }

    MBR mbr;
    fseek(file, 0, SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, file);

    int idx = -1;
    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '1' &&
            strncmp(mbr.mbr_partitions[i].part_name, name.c_str(), 16) == 0) {
            idx = i; break;
        }
    }
    if (idx == -1) { cout << "ERROR: Partición '" << name << "' no encontrada\n"; fclose(file); return; }

    Partition& part = mbr.mbr_partitions[idx];
    long long partStart = part.part_start;
    long long partSize  = part.part_size;

    if (deleteType == "full") {
        const int BUFSIZE = 4096;
        char zeros[BUFSIZE];
        memset(zeros, 0, BUFSIZE);
        long long remaining = partSize;
        fseek(file, partStart, SEEK_SET);
        while (remaining > 0) {
            long long toWrite = (remaining > BUFSIZE) ? BUFSIZE : remaining;
            fwrite(zeros, 1, toWrite, file);
            remaining -= toWrite;
        }
    }

    memset(&part, 0, sizeof(Partition));
    part.part_status = '0';
    fseek(file, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MBR), 1, file);
    fclose(file);

    cout << "OK: Partición '" << name << "' eliminada (" << deleteType << ")\n";
}

// ============================================================
// EXECUTE ADD
// ============================================================
void FDisk::executeAdd(int add, char unit, const string& name,
                        const string& path) {

    if (name.empty() || path.empty()) {
        cout << "ERROR: -name y -path son obligatorios para -add\n"; return;
    }

    long long addBytes = add;
    if      (unit == 'K' || unit == 'k') addBytes *= 1024;
    else if (unit == 'M' || unit == 'm') addBytes *= 1024 * 1024;
    else if (unit != 'B' && unit != 'b') { cout << "ERROR: Unidad inválida\n"; return; }

    FILE* file = fopen(path.c_str(), "rb+");
    if (!file) { cout << "ERROR: No se pudo abrir el disco\n"; return; }

    MBR mbr;
    fseek(file, 0, SEEK_SET);
    fread(&mbr, sizeof(MBR), 1, file);

    int idx = -1;
    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '1' &&
            strncmp(mbr.mbr_partitions[i].part_name, name.c_str(), 16) == 0) {
            idx = i; break;
        }
    }
    if (idx == -1) { cout << "ERROR: Partición '" << name << "' no encontrada\n"; fclose(file); return; }

    Partition& part = mbr.mbr_partitions[idx];
    long long newSize = part.part_size + addBytes;

    if (newSize <= 0) {
        cout << "ERROR: No se puede reducir la partición a tamaño cero o negativo\n";
        fclose(file); return;
    }

    if (addBytes > 0) {
        long long partEnd   = part.part_start + part.part_size;
        long long nextStart = mbr.mbr_tamano;
        const auto& mounted = MountManager::getAll();
        for (int i = 0; i < 4; i++) {
            if (i == idx) continue;
            if (mbr.mbr_partitions[i].part_status == '1') {
                long long s = mbr.mbr_partitions[i].part_start;
                if (s >= partEnd && s < nextStart) {
                    // Only treat as blocker if currently mounted
                    char nameBuf[17] = {};
                    strncpy(nameBuf, mbr.mbr_partitions[i].part_name, 16);
                    string pname(nameBuf);
                    bool isMounted = false;
                    for (const auto& mp : mounted) {
                        if (mp.name == pname && mp.path == path) {
                            isMounted = true; break;
                        }
                    }
                    if (isMounted) nextStart = s;
                }
            }
        }
        long long freeAfter = nextStart - partEnd;
        if (addBytes > freeAfter) {
            cout << "ERROR: No hay espacio libre suficiente después de la partición\n"
                 << "       Espacio disponible: " << freeAfter << " bytes\n";
            fclose(file); return;
        }
    }

    part.part_size = newSize;
    fseek(file, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MBR), 1, file);
    fclose(file);

    if (addBytes >= 0)
        cout << "OK: Partición '" << name << "' ampliada en " << addBytes << " bytes\n";
    else
        cout << "OK: Partición '" << name << "' reducida en " << (-addBytes) << " bytes\n";
}