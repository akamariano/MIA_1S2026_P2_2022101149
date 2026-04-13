#include "commands/mkdisk.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include "disk/mbr.h"

void MkDisk::execute(int size, char unit, std::string fit, std::string path) {

    // Verificar que el tamaño sea válido
    if (size <= 0) {
        std::cout << "ERROR: El tamaño debe ser mayor a 0\n";
        return;
    }

    // Validar que la estrategia de ajuste sea reconocida
    if (fit != "BF" && fit != "FF" && fit != "WF") {
        std::cout << "ERROR: Fit inválido (use BF, FF o WF)\n";
        return;
    }

    // Convertir tamaño a bytes (evitar overflow con long long)
    long long bytes = 0;
    if (unit == 'K' || unit == 'k')
        bytes = (long long)size * 1024;
    else if (unit == 'M' || unit == 'm')
        bytes = (long long)size * 1024 * 1024;
    else {
        std::cout << "ERROR: Unidad no válida\n";
        return;
    }

    // Crear directorios padres si no existen
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
        // Ignorar error si el directorio ya existe
        if (ec && !std::filesystem::exists(p.parent_path())) {
            std::cout << "ERROR: No se pudo crear el directorio: " << ec.message() << "\n";
            return;
        }
    }

    // Avisar si el disco ya existe
    {
        std::ifstream check(path);
        if (check.good()) {
            std::cout << "ADVERTENCIA: El disco ya existe y será sobreescrito\n";
        }
    }

    // Crear y abrir el archivo de disco
    std::fstream disk(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!disk) {
        std::cout << "ERROR: No se pudo crear el archivo en: " << path << "\n";
        return;
    }

    // Llenar todo el disco con ceros
    char buffer[1024] = {0};
    long long remaining = bytes;
    while (remaining > 0) {
        long long writeSize = (remaining > 1024) ? 1024 : remaining;
        disk.write(buffer, writeSize);
        remaining -= writeSize;
    }

    // Escribir el MBR en el inicio del disco
    MBR mbr;
    memset(&mbr, 0, sizeof(MBR));
    mbr.mbr_tamano          = bytes;
    mbr.mbr_fecha_creacion  = time(nullptr);
    mbr.mbr_dsk_signature   = rand();
    strncpy(mbr.dsk_fit, fit.c_str(), 2);
    mbr.dsk_fit[2] = '\0';

    // Inicializar todas las particiones como vacías
    for (int i = 0; i < 4; i++) {
        mbr.mbr_partitions[i].part_status = '0';
        mbr.mbr_partitions[i].part_type   = '0';
        memset(mbr.mbr_partitions[i].part_fit,  0, 3);
        mbr.mbr_partitions[i].part_start  = -1;
        mbr.mbr_partitions[i].part_size   = 0;
        memset(mbr.mbr_partitions[i].part_name, 0, 16);
    }

    disk.seekp(0);
    disk.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    disk.close();

    std::cout << "OK: Disco creado correctamente en " << path 
              << " (" << size << (unit) << ")\n";
}