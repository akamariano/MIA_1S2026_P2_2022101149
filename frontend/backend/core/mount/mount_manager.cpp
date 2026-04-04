#include "mount_manager.h"
#include <iostream>
#include <algorithm>
#include "../disk/mbr.h"
#include <cstring>
#include <map>

std::vector<MountedPartition> MountManager::mountedPartitions;

std::string MountManager::mount(std::string path, std::string name) {

    // Verificar que no esté ya montada
    for (auto &m : mountedPartitions) {
        if (m.path == path && m.name == name) {
            std::cout << "ERROR: La partición ya está montada\n";
            return "";
        }
    }

    // Abrir el disco para leer la tabla de particiones
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        std::cout << "ERROR: No se pudo abrir el disco\n";
        return "";
    }

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    // Buscar la partición por nombre en la tabla del MBR
    long long partStart = -1;
    long long partSize  = -1;

    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_status == '1' &&
            strncmp(mbr.mbr_partitions[i].part_name, name.c_str(), 16) == 0) {
            partStart = mbr.mbr_partitions[i].part_start;
            partSize  = mbr.mbr_partitions[i].part_size;
            break;
        }
    }
    fclose(file);

    if (partStart == -1) {
        std::cout << "ERROR: Partición no encontrada\n";
        return "";
    }

    // Generar ID único para esta partición montada (formato: 49nL donde n es número y L es letra)
    std::string carnet = "49";

    std::map<std::string, char> diskLetter;
    char nextLetter = 'A';

    // Encontrar que letra de disco corresponde a cada disco y la siguiente letra disponible
    for (auto &m : mountedPartitions) {
        if (diskLetter.find(m.path) == diskLetter.end()) {
            diskLetter[m.path] = m.id.back();
        }
        if (m.id.back() >= nextLetter)
            nextLetter = m.id.back() + 1;
    }

    char assignedLetter;
    int  assignedNumber;

    // Asignar número según cuantas particiones del mismo disco ya están montadas
    if (diskLetter.find(path) != diskLetter.end()) {
        assignedLetter = diskLetter[path];
        int count = 0;
        for (auto &m : mountedPartitions)
            if (m.path == path) count++;
        assignedNumber = count + 1;
    } else {
        assignedLetter = nextLetter;
        assignedNumber = 1;
    }

    std::string newId = carnet + std::to_string(assignedNumber) + assignedLetter;

    // Registrar la partición montada
    MountedPartition mp;
    mp.path  = path;
    mp.name  = name;
    mp.id    = newId;
    mp.start = partStart;
    mp.size  = partSize;
    mountedPartitions.push_back(mp);

    std::cout << "OK: Partición montada con ID: " << newId << "\n";
    return newId;
}

MountedPartition* MountManager::getMountedById(std::string id) {
    for (auto &m : mountedPartitions) {
        if (m.id == id) return &m;
    }
    return nullptr;
}

void MountManager::showMounted() {
    std::cout << "\n--- PARTICIONES MONTADAS ---\n";
    if (mountedPartitions.empty()) {
        std::cout << "No hay particiones montadas\n";
        return;
    }
    for (auto &m : mountedPartitions) {
        std::cout << "ID: " << m.id
                  << " | Path: " << m.path
                  << " | Name: " << m.name << "\n";
    }
}

void MountManager::unmountByPath(std::string path) {
    mountedPartitions.erase(
        std::remove_if(mountedPartitions.begin(), mountedPartitions.end(),
            [&path](const MountedPartition& m) { return m.path == path; }),
        mountedPartitions.end()
    );
}
const std::vector<MountedPartition>& MountManager::getAll() {
    return mountedPartitions;
}

void MountManager::unmountById(std::string id) {
    mountedPartitions.erase(
        std::remove_if(mountedPartitions.begin(), mountedPartitions.end(),
            [&id](const MountedPartition& m) { return m.id == id; }),
        mountedPartitions.end()
    );
}