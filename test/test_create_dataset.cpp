#include "core.hpp"
#include "libzfs.h"

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");
        testpool_A.createDataset("testDataset");
    } catch (std::logic_error &e) {
        std::cout << "Caught logic_error!" << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    } catch (std::exception &e) {
        std::cout << "Caught some exception!" << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    }

    std::cout << "I think I made a dataset? :)" << std::endl;

    try {
        // Expects zh to be an initialized handle...
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");
        zfs::Dataset testDataset = testpool_A.openDataset("testDataset");

        std::string targetMountpoint = "/mnt/testMountpointA";

        std::cout << "Trying to set mountpoint to '" << targetMountpoint << "'"
                  << std::endl;
        testDataset.setMountpoint(targetMountpoint);

        std::string mountpointAfter = testDataset.getMountpoint();

        if (targetMountpoint == mountpointAfter) {
            std::cout << "Success!" << std::endl;
        } else {
            std::cout << "Failed setting mountpoint!" << std::endl;
            return -1;
        }

    } catch (std::exception &e) {
        std::cout << "Caught some exception!" << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}
