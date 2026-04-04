#ifndef FDISK_H
#define FDISK_H
#include <string>
#include <cstdio>

class FDisk {
public:
    // Crear partición (original)
    bool existeNombreLogica(FILE* disk, long long startExtendida, std::string nombre);
    void printLogicas(FILE* disk, long long startExtendida);
    void execute(int size, char unit, std::string path,
                 char type, std::string fit, std::string name);

    // Nuevos P2
    void executeDelete(const std::string& deleteType, const std::string& name,
                       const std::string& path);
    void executeAdd(int add, char unit, const std::string& name,
                    const std::string& path);
};
#endif