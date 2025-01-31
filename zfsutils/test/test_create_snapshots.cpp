#include "core.hpp"
#include "libzfs.h"
#include <exception>

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");

        zfs::Dataset testDataset = testpool_A.openDataset("testDataset");
        std::cout << "testDataset zfs name: " << testDataset.name()
                  << std::endl;

        try {
            zfs::Snapshot snap = testDataset.createSnapshot("Hello");
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            return -1;
        }

        try {
            zfs::Snapshot snap = testDataset.openSnapshot("Hello");
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
            return -1;
        }

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}
