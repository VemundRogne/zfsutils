#include "core.hpp"
#include "libzfs.h"

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");

        zfs::Dataset testDataset =
            zfs::ZFS::getDatasetByName("zfsutils_testpool_A/testDataset");

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}
