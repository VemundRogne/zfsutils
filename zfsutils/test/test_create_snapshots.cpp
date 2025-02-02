#include "core.hpp"
#include "libzfs.h"
#include <exception>
#include <filesystem>
#include <fstream>

int main() {
    try {
        zfs::Pool testpool_A = zfs::ZFS::getPoolByName("zfsutils_testpool_A");
        zfs::Dataset testDataset = testpool_A.openDataset("testDataset");

        // Now we want to make a snapshot _before_ doing anything on the
        // dataset. Then we want to 'put' something in the dataset, and then
        // make another snapshot

        testDataset.createSnapshot("initialSnapshot");

        try {
            std::ofstream outFile;
            outFile.exceptions(std::ofstream::badbit | std::ofstream::failbit);
            outFile.open(testDataset.getMountpoint() + "/testfile.txt");
            outFile << "Hello, there! This is some text!" << std::endl;
            outFile.close();

        } catch (const std::ofstream::failure &e) {
            std::cerr << e.what() << std::endl;
            return -1;
        }

        if (!std::filesystem::exists(testDataset.getMountpoint() +
                                     "/testfile.txt")) {
            std::cerr << "Did not manage to make the testfile :(" << std::endl;
            return -1;
        }

        testDataset.createSnapshot("secondSnapshot");

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}
