#ifndef MOUNT_MANAGER_H
#define MOUNT_MANAGER_H

#include <vector>
#include <string>

struct MountedPartition {
    std::string path;
    std::string name;
    std::string id;
    long long start;  
    long long size;
};

class MountManager {
private:
    static std::vector<MountedPartition> mountedPartitions;

public:
    static std::string mount(std::string path, std::string name);
    static void showMounted();
    static MountedPartition* getMountedById(std::string id);
    static void unmountByPath(std::string path);
    static void unmountById(std::string id);
    static const std::vector<MountedPartition>& getAll();
};

#endif